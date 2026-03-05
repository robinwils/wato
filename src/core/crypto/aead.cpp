#include "core/crypto/aead.hpp"

#include <sodium/crypto_aead_aegis256.h>
#include <sodium/crypto_aead_xchacha20poly1305.h>

AEADHandle AEGIS256Handle()
{
    static const AEAD kAEGIS256 = {
        .Encrypt      = crypto_aead_aegis256_encrypt,
        .Decrypt      = crypto_aead_aegis256_decrypt,
        .KeyBytes     = crypto_aead_aegis256_KEYBYTES,
        .NonceBytes   = crypto_aead_aegis256_NPUBBYTES,
        .AuthTagBytes = crypto_aead_aegis256_ABYTES,
    };
    return &kAEGIS256;
}

AEADHandle XChaCha20Poly1305IETFHandle()
{
    static const AEAD kXChaCha20Poly1305IETF = {
        .Encrypt      = crypto_aead_xchacha20poly1305_ietf_encrypt,
        .Decrypt      = crypto_aead_xchacha20poly1305_ietf_decrypt,
        .KeyBytes     = crypto_aead_xchacha20poly1305_ietf_KEYBYTES,
        .NonceBytes   = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES,
        .AuthTagBytes = crypto_aead_xchacha20poly1305_ietf_ABYTES,
    };
    return &kXChaCha20Poly1305IETF;
}
