#include "StdInc.h"
#include "CClientIntegrity.h"

thread_local int CClientIntegrity::ms_iLoadDepth = 0;
thread_local int CClientIntegrity::ms_iStopDepth = 0;

void CClientIntegrity::ReportScriptLoad(const char* szChunkName)
{
    if (g_pClientGame)
        g_pClientGame->TellServerSomethingImportant(1900, SString("L:%s", szChunkName ? szChunkName : "?"));
}

void CClientIntegrity::ReportResourceStop(const char* szResourceName)
{
    if (g_pClientGame)
        g_pClientGame->TellServerSomethingImportant(1900, SString("R:%s", szResourceName ? szResourceName : "?"));
}
