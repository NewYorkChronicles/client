/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  FILE:        multiplayer_sa/CMultiplayerSA_Streaming.cpp
 *
 *  RINWARES IMG: TEA-encrypted directory + per-file XOR data.
 *  All decryption in memory only — files on disk stay encrypted.
 *
 *****************************************************************************/

#include "StdInc.h"
#include <game/CStreaming.h>

void OnModelLoaded(unsigned int uiModelID)
{
    const int32_t baseTxdId = pGameInterface->GetBaseIDforTXD();
    if (baseTxdId <= 0)
        return;
    if (uiModelID < static_cast<unsigned int>(baseTxdId))
    {
        if (auto* pModelInfo = pGameInterface->GetModelInfo(uiModelID))
            pModelInfo->MakeCustomModel();
    }
}

#include "EncryptionKeys.h"

static void TEADecrypt(const uint8_t* in, uint8_t* out, uint32_t size, const uint8_t* key)
{
    uint32_t k[4];
    memcpy(k, key, 16);
    for (uint32_t i = 0; i + 8 <= size; i += 8)
    {
        uint32_t v0, v1;
        memcpy(&v0, in + i, 4);
        memcpy(&v1, in + i + 4, 4);
        uint32_t sum = 0xC6EF3720;
        for (int r = 0; r < 32; r++)
        {
            v1 -= ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
            v0 -= ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
            sum -= 0x9E3779B9;
        }
        memcpy(out + i, &v0, 4);
        memcpy(out + i + 4, &v1, 4);
    }
}

static __forceinline void XORDecrypt(uint8_t* data, uint32_t size, const uint8_t* key16)
{
    uint32_t k[4];
    memcpy(k, key16, 16);
    uint32_t* p = reinterpret_cast<uint32_t*>(data);
    uint32_t full = size >> 4;
    for (uint32_t i = 0; i < full; i++, p += 4)
    {
        p[0] ^= k[0]; p[1] ^= k[1]; p[2] ^= k[2]; p[3] ^= k[3];
    }
    for (uint32_t i = full << 4; i < size; i++)
        data[i] ^= key16[i & 15];
}

//////////////////////////////////////////////////////////////////////////////////////////
// Directory decryption — in memory only, no disk writes.
//
// Hooks 3 CFileMgr::Read call sites inside LoadCdDirectory:
//   0x5B61AB — reads 4-byte magic  (we detect RINWARES here)
//   0x5B61B8 — reads 4-byte count  (we return decrypted count)
//   0x5B61E1 — reads 32-byte entry (we return decrypted entries)
//
// All hooks are plain cdecl functions replacing cdecl calls. No asm.
//////////////////////////////////////////////////////////////////////////////////////////

typedef int (__cdecl *CFileMgr_Read_t)(int, char*, int);
static CFileMgr_Read_t OrigRead = (CFileMgr_Read_t)0x538950;

static bool     s_rinw = false;
static uint32_t s_count = 0;
static uint8_t* s_dir = NULL;
static uint32_t s_pos = 0;

static int __cdecl Hook_ReadMagic(int h, char* buf, int sz)
{
    int ret = OrigRead(h, buf, sz);
    s_rinw = false;

    if (ret < 4 || buf[0] != 'R' || buf[1] != 'I' || buf[2] != 'N' || buf[3] != 'W')
        return ret;

    char ares[4];
    OrigRead(h, ares, 4);
    if (ares[0] != 'A' || ares[1] != 'R' || ares[2] != 'E' || ares[3] != 'S')
        return ret;

    uint8_t encHdr[16];
    OrigRead(h, (char*)encHdr, 16);
    uint8_t decHdr[16];
    TEADecrypt(encHdr, decHdr, 16, VALIDATION_KEY);

    uint32_t check;
    memcpy(&check, decHdr, 4);
    memcpy(&s_count, decHdr + 4, 4);
    if (check != 3071 || s_count == 0)
        return ret;

    uint32_t dirPad = (s_count * 32 + 7) & ~7;
    uint8_t* enc = new uint8_t[dirPad];
    OrigRead(h, (char*)enc, dirPad);

    delete[] s_dir;
    s_dir = new uint8_t[dirPad];
    TEADecrypt(enc, s_dir, dirPad, DATA_KEY);
    delete[] enc;

    s_pos = 0;
    s_rinw = true;
    return ret;
}

static int __cdecl Hook_ReadCount(int h, char* buf, int sz)
{
    if (s_rinw)
    {
        memcpy(buf, &s_count, 4);
        return 4;
    }
    return OrigRead(h, buf, sz);
}

static int __cdecl Hook_ReadEntry(int h, char* buf, int sz)
{
    if (s_rinw && s_dir && s_pos + 32 <= s_count * 32)
    {
        memcpy(buf, s_dir + s_pos, 32);
        s_pos += 32;
        if (s_pos >= s_count * 32)
        {
            delete[] s_dir;
            s_dir = NULL;
            s_rinw = false;
        }
        return 32;
    }
    return OrigRead(h, buf, sz);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Per-file data decryption in ConvertBufferToObject
//////////////////////////////////////////////////////////////////////////////////////////
static void DecryptStreamingBuffer(char* pBuffer, int)
{
    uint8_t* buf = reinterpret_cast<uint8_t*>(pBuffer);
    if (*reinterpret_cast<uint32_t*>(buf) != 0x574E4952)
        return;
    if (*reinterpret_cast<uint32_t*>(buf + 4) != 0x53455241)
        return;

    uint32_t encDataSize;
    memcpy(&encDataSize, buf + 8, 4);

    uint8_t fileHeader[32];
    TEADecrypt(buf + 12, fileHeader, 32, HEADER_KEY);

    uint32_t flags;
    memcpy(&flags, fileHeader + 4, 4);
    uint8_t dataKey[16];
    memcpy(dataKey, fileHeader + 8, 16);

    uint8_t* encData = buf + 44;
    if (flags & 8)
        XORDecrypt(encData, encDataSize, dataKey);
    else if (flags & 1)
        TEADecrypt(encData, encData, encDataSize, dataKey);

    memmove(buf, encData, encDataSize);
}

#define HOOKPOS_CStreaming__ConvertBufferToObject_Pre  0x40C6B0
#define HOOKSIZE_CStreaming__ConvertBufferToObject_Pre 7
static const DWORD CONTINUE_ConvertBufferToObject = 0x40C6B7;

static void _declspec(naked) HOOK_CStreaming__ConvertBufferToObject_Pre()
{
    __asm
    {
        sub     esp, 20h
        mov     ecx, [esp+24h]
        pushad
        mov     eax, [esp+48h]
        push    eax
        mov     ecx, [esp+48h]
        push    ecx
        call    DecryptStreamingBuffer
        add     esp, 8
        popad
        jmp     CONTINUE_ConvertBufferToObject
    }
}

#define HOOKPOS_CStreaming__ConvertBufferToObject  0x40CB88
#define HOOKSIZE_CStreaming__ConvertBufferToObject 9

static void _declspec(naked) HOOK_CStreaming__ConvertBufferToObject()
{
    __asm
    {
        push    esi
        call    OnModelLoaded
        pop     esi
        pop     edi
        pop     esi
        pop     ebp
        mov     al, 1
        pop     ebx
        add     esp, 20h
        retn
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Patch a call site: replace target of existing E8 call instruction
//////////////////////////////////////////////////////////////////////////////////////////
static void PatchCall(DWORD addr, DWORD target)
{
    DWORD oldProt;
    VirtualProtect((void*)addr, 5, PAGE_EXECUTE_READWRITE, &oldProt);
    *(DWORD*)(addr + 1) = target - addr - 5;
    VirtualProtect((void*)addr, 5, oldProt, &oldProt);
}

void CMultiplayerSA::InitHooks_Streaming()
{
    EZHookInstall(CStreaming__ConvertBufferToObject);
    EZHookInstall(CStreaming__ConvertBufferToObject_Pre);

    PatchCall(0x5B61AB, (DWORD)&Hook_ReadMagic);
    PatchCall(0x5B61B8, (DWORD)&Hook_ReadCount);
    PatchCall(0x5B61E1, (DWORD)&Hook_ReadEntry);
}
