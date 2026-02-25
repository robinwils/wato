#pragma once

#include <array>
#include <cstddef>

struct AEAD {
    int (*Encrypt)(
        unsigned char*       c,
        unsigned long long*  clen_p,
        const unsigned char* m,
        unsigned long long   mlen,
        const unsigned char* ad,
        unsigned long long   adlen,
        const unsigned char* nsec,
        const unsigned char* npub,
        const unsigned char* k);
    int (*Decrypt)(
        unsigned char*       m,
        unsigned long long*  mlen_p,
        unsigned char*       nsec,
        const unsigned char* c,
        unsigned long long   clen,
        const unsigned char* ad,
        unsigned long long   adlen,
        const unsigned char* npub,
        const unsigned char* k);

    std::size_t KeyBytes;
    std::size_t NonceBytes;
    std::size_t AuthTagBytes;
};

using AEADHandle = const AEAD*;

AEADHandle AEGIS256Handle();
AEADHandle XChaCha20Poly1305IETFHandle();
