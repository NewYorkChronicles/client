/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CStreamingSA.h
 *  PURPOSE:     Header file for data streamer class
 *
 *  Multi Theft Auto is available from https://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

#include <array>
#include <map>
#include <game/CStreaming.h>

#define VAR_DefaultStreamHandlersMaxCount 32
#define VAR_DefaultMaxArchives            32

#define FUNC_CStreaming__RequestModel            0x4087E0
#define FUNC_LoadAllRequestedModels              0x40EA10
#define FUNC_CStreaming__HasVehicleUpgradeLoaded 0x407820
#define FUNC_CStreaming_RequestSpecialModel      0x409d10
#define FUNC_CStreaming_LoadScene                0x40EB70
#define FUNC_CStreaming_LoadSceneCollision       0x40ED80

struct CArchiveInfo
{
    char  szName[40];
    BYTE  bUnknow = 1;  // Only in player.img is 0. Maybe, it is DWORD value
    BYTE  bUnused[3];
    DWORD uiStreamHandleId{};
};

// Get player.img archive index dynamically (not hardcoded to 5)
// Searches CArchiveInfo array for "player.img" entry
inline int GetPlayerImgArchiveIndex()
{
    static int s_cachedIndex = -2;  // -2 = not initialized
    if (s_cachedIndex != -2)
        return s_cachedIndex;

    CArchiveInfo* pImgs = (CArchiveInfo*)0x8E48D8;
    for (int i = 0; i < 32; i++)
    {
        if (pImgs[i].szName[0])
        {
            size_t len = strlen(pImgs[i].szName);
            if (len >= 10 && _stricmp(pImgs[i].szName + len - 10, "player.img") == 0)
            {
                s_cachedIndex = i;
                return i;
            }
        }
    }
    s_cachedIndex = 5;  // Fallback to stock
    return 5;
}

struct SGtaStream
{
    uint32_t nSectorsOffset;
    uint32_t nSectorsToRead;
    void*    pBuffer;
    uint8_t  bUnknow1;
    uint8_t  bLocked;
    uint8_t  bInUse;
    uint8_t  bUnknow2;
    uint32_t uiStatus;
    uint32_t handle;
    uint32_t file;
    uint8_t  pad[20];
};
static_assert(sizeof(SGtaStream) == 0x30, "Invalid size for SGtaStream");

struct SStreamName
{
    char szName[64];
};

class CStreamingSA final : public CStreaming
{
public:
    CStreamingSA();

    void SetArchivesNum(size_t imagesNum);

    void RequestModel(DWORD dwModelID, DWORD dwFlags);
    void RemoveModel(std::uint32_t model) override;
    void LoadAllRequestedModels(bool bOnlyPriorityModels = false, const char* szTag = NULL);
    bool HasModelLoaded(DWORD dwModelID);
    void RequestSpecialModel(DWORD model, const char* szTexture, DWORD channel);
    void ReinitStreaming();
    void RemoveBigBuildings() override;

    CStreamingInfo* GetStreamingInfo(uint32 id);
    void            SetStreamingInfo(uint32 modelid, unsigned char usStreamID, uint uiOffset, ushort usSize, uint uiNextInImg = -1);
    unsigned char   GetUnusedArchive();
    unsigned char   GetUnusedStreamHandle();
    unsigned char   AddArchive(const wchar_t* szFilePath);
    void            RemoveArchive(unsigned char ucStreamHandler);
    bool            SetStreamingBufferSize(uint32 uiSize);
    uint32          GetStreamingBufferSize() { return ms_streamingHalfOfBufferSizeBlocks * 2048 * 2; };  // In bytes

    void          MakeSpaceFor(std::uint32_t memoryToCleanInBytes) override;
    std::uint32_t GetMemoryUsed() const override;

    void LoadScene(const CVector* position);
    void LoadSceneCollision(const CVector* position);

    static CStreamingInfo* ms_aInfoForModel;       // Dynamically resolved — FLA may relocate this array
    static uint32_t        ms_aInfoForModelCount;  // Actual count — read from FLA-patched immediates

private:
    void AllocateArchive();
    void BuildChainCache();
    void DetachFromChain(uint32_t modelId);
    void InsertIntoChain(uint32_t modelId);

    std::vector<CArchiveInfo> m_Imgs;
    std::vector<HANDLE>       m_StreamHandles;
    std::vector<SStreamName>  m_StreamNames;

    std::array<std::map<uint32_t, uint32_t>, 256> m_ChainIndex;
    bool                                          m_bChainCacheBuilt = false;

    static void* (&ms_pStreamingBuffer)[2];
    static uint32(&ms_streamingHalfOfBufferSizeBlocks);
};
