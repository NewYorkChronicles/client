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

    bool HasNycProtocol(const char* cmdLine)
    {
        if (!cmdLine) return false;
        const char* p = strstr(cmdLine, "nyc://");
        if (!p) return false;
        p += 6;
        return (strstr(p, "newyorkchronicles") != nullptr ||
                strstr(p, "play.newyorkchronicles") != nullptr ||
                strstr(p, "editor.newyorkchronicles") != nullptr);
    }

    bool IsInstallerCommand(const char* cmdLine)
    {
        if (!cmdLine) return false;
        return (strstr(cmdLine, "/kdinstall") || strstr(cmdLine, "/kduninstall"));
    }

    DWORD GetParentPid()
    {
        DWORD pid = GetCurrentProcessId();
        DWORD ppid = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32 pe = {};
        pe.dwSize = sizeof(pe);
        if (Process32First(snap, &pe))
        {
            do {
                if (pe.th32ProcessID == pid) { ppid = pe.th32ParentProcessID; break; }
            } while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
        return ppid;
    }

    bool IsParentLauncher()
    {
        DWORD ppid = GetParentPid();
        if (ppid == 0) return false;
        HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ppid);
        if (!proc) return false;
        char path[MAX_PATH] = {};
        DWORD sz = MAX_PATH;
        bool result = false;
        if (QueryFullProcessImageNameA(proc, 0, path, &sz))
        {
            _strlwr_s(path, sz + 1);
            result = (strstr(path, "launcher.exe") != nullptr);
        }
        CloseHandle(proc);
        return result;
    }

    bool IsAuthorizedLaunch(const char* cmdLine)
    {
        if (IsInstallerCommand(cmdLine))
            return true;
        if (HasNycProtocol(cmdLine))
            return true;
        if (IsParentLauncher())
            return true;
        return false;
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
    CheckAntiVirusStatus();
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
