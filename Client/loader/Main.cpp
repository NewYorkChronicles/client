/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Client/loader/Main.cpp
 *  PURPOSE:     MTA loader
 *
 *  Multi Theft Auto is available from https://multitheftauto.com/
 *
 *****************************************************************************/

#include "Main.h"
#include "CInstallManager.h"
#include "MainFunctions.h"
#include "Dialogs.h"
#include "Utils.h"
#include "SharedUtil.Win32Utf8FileHooks.hpp"

#if defined(MTA_DEBUG)
    #include "SharedUtil.Tests.hpp"
#endif

#include <version.h>
#include <memory>
#include <algorithm>
#include <cassert>
#include <tlhelp32.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#pragma comment(lib, "wintrust")
#pragma comment(lib, "crypt32")

#if __cplusplus >= 201703L
    #define MAYBE_UNUSED [[maybe_unused]]
#else
    #define MAYBE_UNUSED
#endif

namespace
{
    enum ErrorCode : int
    {
        ERROR_NULL_INSTANCE = -1,
        ERROR_NULL_INSTALL_MANAGER = -2,
        ERROR_LAUNCH_EXCEPTION = -3,
        ERROR_INSTALL_CONTINUE = -4,
        ERROR_NOT_AUTHORIZED = -5
    };

    constexpr size_t MAX_CMD_LINE_LENGTH = 4096;
    constexpr int LOG_ID_END = 1044;
    constexpr int LOG_ID_CONTINUE_EXCEPTION = 1045;
    constexpr int LOG_ID_LAUNCH_EXCEPTION = 1046;

    static_assert(MAX_CMD_LINE_LENGTH > 0, "Command line buffer size must be positive");

    class Utf8FileHooksGuard
    {
        bool m_released = false;
    public:
        Utf8FileHooksGuard() { AddUtf8FileHooks(); }
        ~Utf8FileHooksGuard() noexcept { if (!m_released) RemoveUtf8FileHooks(); }
        void release() noexcept { m_released = true; }
        void removeNow() noexcept { if (!m_released) { RemoveUtf8FileHooks(); m_released = true; } }
        Utf8FileHooksGuard(const Utf8FileHooksGuard&) = delete;
        Utf8FileHooksGuard& operator=(const Utf8FileHooksGuard&) = delete;
    };

    inline void SafeCopyCommandLine(LPSTR lpCmdLine, char* dest, size_t bufSize) noexcept
    {
        if (!dest || bufSize == 0) return;
        if (!lpCmdLine) return;
        size_t len = strnlen(lpCmdLine, bufSize - 1);
        memcpy(dest, lpCmdLine, len);
        dest[len] = '\0';
    }

    bool IsInstallerCommand(const char* cmdLine)
    {
        if (!cmdLine) return false;
        return (strstr(cmdLine, "/kdinstall") || strstr(cmdLine, "/kduninstall"));
    }

    bool VerifyTrust(const wchar_t* path)
    {
        WINTRUST_FILE_INFO fi = {};
        fi.cbStruct = sizeof(fi);
        fi.pcwszFilePath = path;
        WINTRUST_DATA wd = {};
        wd.cbStruct = sizeof(wd);
        wd.dwUIChoice = WTD_UI_NONE;
        wd.fdwRevocationChecks = WTD_REVOKE_NONE;
        wd.dwUnionChoice = WTD_CHOICE_FILE;
        wd.pFile = &fi;
        wd.dwStateAction = WTD_STATEACTION_VERIFY;
        GUID guid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
        LONG status = WinVerifyTrust(nullptr, &guid, &wd);
        wd.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust(nullptr, &guid, &wd);
        return status == ERROR_SUCCESS;
    }

    bool SignerIsNyc(const wchar_t* path)
    {
        DWORD      enc = 0, ctype = 0, ftype = 0;
        HCERTSTORE store = nullptr;
        HCRYPTMSG  msg = nullptr;
        if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE, path, CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED, CERT_QUERY_FORMAT_FLAG_BINARY, 0, &enc, &ctype, &ftype,
                              &store, &msg, nullptr))
            return false;
        bool  ok = false;
        DWORD size = 0;
        if (CryptMsgGetParam(msg, CMSG_SIGNER_INFO_PARAM, 0, nullptr, &size) && size)
        {
            if (auto* signer = static_cast<CMSG_SIGNER_INFO*>(LocalAlloc(LPTR, size)))
            {
                if (CryptMsgGetParam(msg, CMSG_SIGNER_INFO_PARAM, 0, signer, &size))
                {
                    CERT_INFO ci = {};
                    ci.Issuer = signer->Issuer;
                    ci.SerialNumber = signer->SerialNumber;
                    if (PCCERT_CONTEXT cert = CertFindCertificateInStore(store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, &ci, nullptr))
                    {
                        wchar_t cn[256] = {};
                        if (CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, cn, 256) > 1)
                            ok = (wcsstr(cn, L"Mohamed Rayane Merzoug") != nullptr);
                        CertFreeCertificateContext(cert);
                    }
                }
                LocalFree(signer);
            }
        }
        if (msg) CryptMsgClose(msg);
        if (store) CertCloseStore(store, 0);
        return ok;
    }

    bool IsSignedLauncherRunning()
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;
        bool            found = false;
        PROCESSENTRY32W pe = {};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe))
        {
            do {
                if (_wcsicmp(pe.szExeFile, L"launcher.exe") != 0) continue;
                if (HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID))
                {
                    wchar_t path[MAX_PATH] = {};
                    DWORD   len = MAX_PATH;
                    if (QueryFullProcessImageNameW(proc, 0, path, &len) && VerifyTrust(path) && SignerIsNyc(path))
                        found = true;
                    CloseHandle(proc);
                }
            } while (!found && Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return found;
    }

    bool IsAuthorizedLaunch(const char* cmdLine)
    {
#if defined(MTA_DEBUG)
        return true;
#else
        // Loader's own admin self-relaunch carries /nyc-admin and is always elevated; the launcher never starts the game elevated
        if (cmdLine && strstr(cmdLine, "/nyc-admin") && IsUserAdmin())
            return true;
        return IsInstallerCommand(cmdLine) || IsSignedLauncherRunning();
#endif
    }

    CInstallManager* PerformEarlyInitialization(const char* safeCmdLine)
    {
        auto* pInstallManager = GetInstallManager();
        if (!pInstallManager) return nullptr;

        pInstallManager->SetMTASAPathSource(safeCmdLine);
        ConfigureWerDumpPath();
        BeginEventLog();
        InitLocalization(false);
        HandleSpecialLaunchOptions();

        if (!IsAuthorizedLaunch(safeCmdLine))
            ExitProcess(0);

        HandleDuplicateLaunching();
        return pInstallManager;
    }

    SString ContinueUpdateProcedure(CInstallManager* pInstallManager)
    {
        if (!pInstallManager) return SString();
        try { return pInstallManager->Continue(); }
        catch (...) { AddReportLog(LOG_ID_CONTINUE_EXCEPTION, "Exception in InstallManager::Continue()"); return SString(); }
    }

    int LaunchGameSafely(const SString& strCmdLine)
    {
        try { return LaunchGame(strCmdLine); }
        catch (...) { AddReportLog(LOG_ID_LAUNCH_EXCEPTION, "Exception in LaunchGame()"); return static_cast<int>(ERROR_LAUNCH_EXCEPTION); }
    }
}

MTAEXPORT int DoWinMain(HINSTANCE hLauncherInstance, MAYBE_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, MAYBE_UNUSED int nCmdShow)
{
#if __cplusplus < 201703L
    (void)hPrevInstance;
    (void)nCmdShow;
#endif

    if (!hLauncherInstance) return static_cast<int>(ERROR_NULL_INSTANCE);

    char safeCmdLine[MAX_CMD_LINE_LENGTH] = {};
    SafeCopyCommandLine(lpCmdLine, safeCmdLine, sizeof(safeCmdLine));

    Utf8FileHooksGuard utf8Guard;

#if defined(MTA_DEBUG)
    SharedUtil_Tests();
#endif

    auto* pInstallManager = PerformEarlyInitialization(safeCmdLine);
    if (!pInstallManager) { utf8Guard.removeNow(); return static_cast<int>(ERROR_NULL_INSTALL_MANAGER); }

    ShowSplash(hLauncherInstance);
    ClearPendingBrowseToSolution();
    ValidateGTAPath();

    SString strCmdLine = ContinueUpdateProcedure(pInstallManager);

    InitLocalization(true);
    PreLaunchWatchDogs();
    HandleCustomStartMessage();

#if !defined(MTA_DEBUG) && MTASA_VERSION_TYPE != VERSION_TYPE_CUSTOM
    ForbodenProgramsMessage();
#endif

    CycleEventLog();
    BsodDetectionPreLaunch();
    MaybeShowCopySettingsDialog();
    HandleIfGTAIsAlreadyRunning();
    ShowSplash(hLauncherInstance);
    CheckDataFiles();
    CheckLibVersions();

    int iReturnCode = LaunchGameSafely(strCmdLine);
    PostRunWatchDogs(iReturnCode);
    HandleOnQuitCommand();
    ProcessPendingBrowseToSolution();

    AddReportLog(LOG_ID_END, SString("* End (0x%08X)* pid:%lu",
        static_cast<DWORD>(static_cast<unsigned int>(iReturnCode)),
        static_cast<unsigned long>(GetCurrentProcessId())));

    return iReturnCode;
}
