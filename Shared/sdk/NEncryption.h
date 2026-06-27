#pragma once
#include <cstdint>
#include <cstring>

namespace NEncryption
{
    inline void DeriveHeaderKey(uint8_t out[16])
    {
        volatile uint32_t t[4];
        t[0] = 0x0769CB8Fu ^ 0xB0A11119u;
        t[2] = 0x6843AE1Au ^ 0xC2A93C5Au;
        t[1] = 0x8D04B3F6u ^ 0xF650244Cu;
        t[3] = 0x5731E4D4u ^ 0x22F1A5CCu;
        memcpy(out, (void*)t, 16);
    }

    inline void DeriveValidationKey(uint8_t out[16])
    {
        volatile uint32_t t[4];
        t[1] = 0x11BE903Cu ^ 0xBD4B8D25u;
        t[3] = 0x27362050u ^ 0xF8A44024u;
        t[0] = 0x80B4D7BFu ^ 0xE52B6AD0u;
        t[2] = 0x0F393819u ^ 0xDB5257BFu;
        memcpy(out, (void*)t, 16);
    }

    inline void DeriveDataKey(uint8_t out[16])
    {
        volatile uint32_t t[4];
        t[2] = 0x5B70BF59u ^ 0x3E5F0EF6u;
        t[0] = 0x2AC8C5C6u ^ 0xDA20703Du;
        t[3] = 0x9605685Du ^ 0x43D19C58u;
        t[1] = 0xD5C4FF6Eu ^ 0x61BF5EAEu;
        memcpy(out, (void*)t, 16);
    }

    inline void TEADecrypt(const uint8_t* in, uint8_t* out, uint32_t size, const uint8_t* key)
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

    inline void XORDecrypt(uint8_t* data, uint32_t size, const uint8_t* key16)
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
}
