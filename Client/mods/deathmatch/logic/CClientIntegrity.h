#pragma once

#ifdef _WIN32
extern "C" __declspec(dllimport) int g_NycLuaAuthDepth;
#else
extern "C" int g_NycLuaAuthDepth;
#endif

class CClientIntegrity
{
public:
    class CScriptLoadScope
    {
    public:
        CScriptLoadScope()  { ++ms_iLoadDepth;  ++g_NycLuaAuthDepth; }
        ~CScriptLoadScope() { --ms_iLoadDepth;  --g_NycLuaAuthDepth; }
    };

    class CResourceStopScope
    {
    public:
        CResourceStopScope()  { ++ms_iStopDepth; }
        ~CResourceStopScope() { --ms_iStopDepth; }
    };

    static bool IsAuthorizedScriptLoad()   { return ms_iLoadDepth > 0; }
    static bool IsAuthorizedResourceStop() { return ms_iStopDepth > 0; }

    static void ReportScriptLoad(const char* szChunkName);
    static void ReportResourceStop(const char* szResourceName);

private:
    static thread_local int ms_iLoadDepth;
    static thread_local int ms_iStopDepth;
};
