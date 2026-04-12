/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        multiplayer_sa/multiplayersa_init.cpp
 *  PURPOSE:     Multiplayer module entry
 *
 *  Multi Theft Auto is available from https://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include "SharedUtil.Memory.h"
#define DECLARE_PROFILER_SECTION_multiplayersa_init
#include "profiler/SharedUtil.Profiler.h"

CGame*          pGameInterface = 0;
CMultiplayerSA* pMultiplayer = 0;
CNet*           g_pNet = NULL;
CCoreInterface* g_pCore = NULL;

//-----------------------------------------------------------
// This function uses the initialized data sections of the executables
// to differentiate between versions.  MUST be called at least once
// in order for proper initialization to occur.

MTAEXPORT CMultiplayer* InitMultiplayerInterface(CCoreInterface* pCore)
{
    // set the internal pointer to the game class
    pGameInterface = pCore->GetGame();
    g_pNet = pCore->GetNetwork();
    g_pCore = pCore;
    assert(pGameInterface);
    assert(g_pNet);

    SetMemoryAllocationFailureHandler();

    // create an instance of our multiplayer class
    pMultiplayer = new CMultiplayerSA;
    pMultiplayer->InitHooks();

    // return the multiplayer class ptr
    return (CMultiplayer*)pMultiplayer;
}

//-----------------------------------------------------------

void MemSet(void* dwDest, int cValue, uint uiAmount)
{
    if (ismemset(dwDest, cValue, uiAmount))
        return;
    SMemWrite hMem = OpenMemWrite(dwDest, uiAmount);
    memset(dwDest, cValue, uiAmount);
    CloseMemWrite(hMem);
}

void MemCpy(void* dwDest, const void* dwSrc, uint uiAmount)
{
    if (memcmp(dwDest, dwSrc, uiAmount) == 0)
        return;
    SMemWrite hMem = OpenMemWrite(dwDest, uiAmount);
    memcpy(dwDest, dwSrc, uiAmount);
    CloseMemWrite(hMem);
}

void OnCrashAverted(uint uiId)
{
    g_pCore->OnCrashAverted(uiId);
}

void OnEnterCrashZone(uint uiId)
{
    g_pCore->OnEnterCrashZone(uiId);
}

bool GetDebugIdEnabled(uint uiDebugId)
{
    return g_pCore->GetDebugIdEnabled(uiDebugId);
}

void LogEvent(uint uiDebugId, const char* szType, const char* szContext, const char* szBody, uint uiAddReportLogId)
{
    g_pCore->LogEvent(uiDebugId, szType, szContext, szBody, uiAddReportLogId);
}

void CallGameEntityRenderHandler(CEntitySAInterface* pEntity)
{
    // Only call if not a dummy
    if (!pEntity || pEntity->nType != ENTITY_TYPE_DUMMY)
        if (pGameEntityRenderHandler)
            pGameEntityRenderHandler(pEntity);
}

void OnRequestStreamingMemoryRelief(std::uint32_t bytesNeeded)
{
    if (!pGameInterface)
        return;

    auto* pStreaming = pGameInterface->GetStreaming();
    if (!pStreaming)
        return;

    pStreaming->MakeSpaceFor(bytesNeeded);
}

static volatile bool s_bStreamingReliefRequested = false;

void OnRequestDeferredStreamingMemoryRelief()
{
    s_bStreamingReliefRequested = true;
}

void ProcessDeferredStreamingMemoryRelief()
{
    if (!s_bStreamingReliefRequested)
        return;

    s_bStreamingReliefRequested = false;
    OnRequestStreamingMemoryRelief(4 * 1024 * 1024);
}

// One-shot: double the streaming buffer after GTA's Init2 allocates it.
// A larger buffer means fewer files trigger big-model mode (which halves I/O throughput).
// Costs ~30MB extra RAM but significantly reduces streaming stalls for large TXDs.
void ProcessStreamingBufferUpgrade()
{
    static bool s_bDone = false;
    if (s_bDone)
        return;

    constexpr DWORD VAR_streamingHalfBufferBlocks = 0x8E4CA8;
    auto halfBlocks = *reinterpret_cast<volatile uint32_t*>(VAR_streamingHalfBufferBlocks);
    if (halfBlocks == 0)
        return;

    s_bDone = true;

    auto* pStreaming = pGameInterface->GetStreaming();
    if (!pStreaming)
        return;

    pStreaming->SetStreamingBufferSize(halfBlocks * 4);
}

// Proactive streaming memory pressure management.
// Evicts least-used models gradually when memory usage gets high, preventing
// the engine from doing sudden mass-unloads that cause visible pop-out.
// NOTE: 0x8A5A80 stores bytes (not KB). Previous code had a * 1024 bug that
// made the budget appear 1024x larger, so the GC never triggered.
void ProcessProactiveStreamingCleanup()
{
    constexpr DWORD VAR_memoryUsed      = 0x8E4CB4;
    constexpr DWORD VAR_memoryAvailable = 0x8A5A80;  // bytes, NOT KB

    const auto memUsed  = *reinterpret_cast<volatile DWORD*>(VAR_memoryUsed);
    const auto memAvail = *reinterpret_cast<volatile DWORD*>(VAR_memoryAvailable);

    if (memAvail == 0)
        return;

    static uint32_t s_skipFrames = 0;
    if (s_skipFrames > 0)
    {
        --s_skipFrames;
        return;
    }

    using RemoveLeastUsedModel_t = bool(__cdecl*)(int);
    auto RemoveLeastUsedModel = reinterpret_cast<RemoveLeastUsedModel_t>(0x40CFD0);

    const float usagePercent = (static_cast<float>(memUsed) / static_cast<float>(memAvail)) * 100.0f;

    int evictCount = 0;

    if (usagePercent >= 95.0f)
        evictCount = 2;
    else if (usagePercent >= 90.0f)
        evictCount = 1;

    if (evictCount == 0)
        return;

    for (int i = 0; i < evictCount; i++)
        RemoveLeastUsedModel(0);

    s_skipFrames = 4;
}

// Auto-scale streaming memory budget based on system RAM.
// Budget = clamp(totalRAM / 16, 128MB, 768MB).
// Uses g_pCore->SetCustomStreamingMemory() so MTA's DoPulsePostFrame
// pipeline applies our budget via CLimitsSA::SetStreamingMemory instead
// of us fighting with the CVAR system over 0x8A5A80.
void ProcessStreamingMemoryAutoScale()
{
    static bool s_bDone = false;
    if (s_bDone)
        return;

    if (!g_pCore)
        return;

    MEMORYSTATUSEX memInfo = {};
    memInfo.dwLength = sizeof(memInfo);

    size_t budget;
    if (!GlobalMemoryStatusEx(&memInfo))
    {
        budget = 128u * 1024u * 1024u;  // fallback 128MB
    }
    else
    {
        uint64_t totalBytes = memInfo.ullTotalPhys;
        uint64_t target = totalBytes / 16;

        constexpr uint64_t MIN_BUDGET = 128ull * 1024ull * 1024ull;
        constexpr uint64_t MAX_BUDGET = 768ull * 1024ull * 1024ull;

        if (target < MIN_BUDGET) target = MIN_BUDGET;
        if (target > MAX_BUDGET) target = MAX_BUDGET;

        budget = static_cast<size_t>(target);
    }

    g_pCore->SetCustomStreamingMemory(budget);
    s_bDone = true;
}

// Enforce higher draw distances every frame.
// - Global LOD multiplier (ms_lodDistScale) set to 3.0 (stock: 0.925-1.8)
// - Vehicle/Ped/Train LOD distances raised via CSettingsSA's patched float pointers
void EnforceDrawDistances()
{
    static bool s_bDone = false;
    if (s_bDone)
        return;
    s_bDone = true;
    constexpr DWORD ADDR_lodDistScale = 0x8CD800;
    MemPutFast<float>(ADDR_lodDistScale, 2.5f);
}

