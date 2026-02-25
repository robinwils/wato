#pragma once

#include <sodium/crypto_kx.h>
#include <sodium/randombytes.h>

#include <cstring>
#include <span>
#include <vector>

#include "core/crypto/aead.hpp"
#include "core/crypto/key.hpp"
#include "core/types.hpp"

#define MAX_NONCE_BYTES 32U

class CryptoSession
{
   public:
    void Init(const CryptoKeys& aSelf, CryptoKeys::Public aPub, bool aHasAESNI, bool aServer)
    {
        mAEGIS = aHasAESNI;
        mAEAD  = aHasAESNI ? AEGIS256Handle() : XChaCha20Poly1305IETFHandle();
        if (aServer) {
            mValid =
                (crypto_kx_server_session_keys(
                     mRX,
                     mTX,
                     aSelf.PublicKey().data(),
                     aSelf.SecretKey().data(),
                     aPub.data())
                 == 0);
        } else {
            mValid =
                (crypto_kx_client_session_keys(
                     mRX,
                     mTX,
                     aSelf.PublicKey().data(),
                     aSelf.SecretKey().data(),
                     aPub.data())
                 == 0);
        }
    }

    bool Valid() const { return mValid; }

    byte_view Encrypt(byte_view aBytes);
    byte_view Decrypt(byte_view aBytes);

   private:
    unsigned char mRX[crypto_kx_SESSIONKEYBYTES]{}, mTX[crypto_kx_SESSIONKEYBYTES]{};
    AEADHandle    mAEAD;
    bool          mValid{false};
    bool          mAEGIS{false};

    std::vector<uint8_t> mBuffer{};
};
