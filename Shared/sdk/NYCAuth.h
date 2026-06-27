#pragma once
#include <cstdint>
#include <cstring>

namespace NYCAuth
{
    inline void TEAEncrypt(const uint8_t* in, uint8_t* out, uint32_t size, const uint8_t* key)
    {
        uint32_t k[4];
        memcpy(k, key, 16);
        for (uint32_t i = 0; i + 8 <= size; i += 8)
        {
            uint32_t v0, v1;
            memcpy(&v0, in + i, 4);
            memcpy(&v1, in + i + 4, 4);
            uint32_t sum = 0;
            for (int r = 0; r < 32; r++)
            {
                sum += 0x9E3779B9;
                v0 += ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
                v1 += ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
            }
            memcpy(out + i, &v0, 4);
            memcpy(out + i + 4, &v1, 4);
        }
    }

    inline void DeriveKey(uint8_t out[16])
    {
        volatile uint32_t t[4];
        t[1] = 0xA61929A8u ^ 0x3CF04D1Cu;
        t[3] = 0x937C0D35u ^ 0xB7F2067Bu;
        t[0] = 0xE80B41DDu ^ 0xAAE730FDu;
        t[2] = 0x2BFB5EE7u ^ 0xDB52A0E2u;
        memcpy(out, (void*)t, 16);
    }
}
