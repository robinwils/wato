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
                     aSelf.RawPublicKey().data(),
                     aSelf.RawSecretKey().data(),
                     aPub.data())
                 == 0);
        } else {
            mValid =
                (crypto_kx_client_session_keys(
                     mRX,
                     mTX,
                     aSelf.RawPublicKey().data(),
                     aSelf.RawSecretKey().data(),
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

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>

TEST_CASE("crypto.session_aead_round_trip")
{
    CryptoKeys clientKeys;
    CryptoKeys serverKeys;

    CryptoSession clientSession;
    CryptoSession serverSession;

    serverSession.Init(serverKeys, clientKeys.RawPublicKey(), false, true);
    clientSession.Init(clientKeys, serverKeys.RawPublicKey(), false, false);

    REQUIRE(serverSession.Valid());
    REQUIRE(clientSession.Valid());

    SUBCASE("client to server")
    {
        std::vector<uint8_t> plaintext = {0xDE, 0xAD, 0xBE, 0xEF};

        byte_view encrypted = clientSession.Encrypt(plaintext);
        REQUIRE_FALSE(encrypted.empty());
        CHECK(encrypted.size() > plaintext.size());

        byte_view decrypted = serverSession.Decrypt(encrypted);
        REQUIRE_FALSE(decrypted.empty());
        CHECK(std::vector<uint8_t>(decrypted.begin(), decrypted.end()) == plaintext);
    }

    SUBCASE("server to client")
    {
        std::vector<uint8_t> plaintext = {0xCA, 0xFE, 0xBA, 0xBE};

        byte_view encrypted = serverSession.Encrypt(plaintext);
        REQUIRE_FALSE(encrypted.empty());

        byte_view decrypted = clientSession.Decrypt(encrypted);
        REQUIRE_FALSE(decrypted.empty());
        CHECK(std::vector<uint8_t>(decrypted.begin(), decrypted.end()) == plaintext);
    }
}

TEST_CASE("crypto.session_aead_tampered")
{
    CryptoKeys clientKeys;
    CryptoKeys serverKeys;

    CryptoSession clientSession;
    CryptoSession serverSession;

    serverSession.Init(serverKeys, clientKeys.RawPublicKey(), false, true);
    clientSession.Init(clientKeys, serverKeys.RawPublicKey(), false, false);

    std::vector<uint8_t> plaintext = {1, 2, 3, 4, 5};

    byte_view encrypted = clientSession.Encrypt(plaintext);
    REQUIRE_FALSE(encrypted.empty());

    std::vector<uint8_t> tampered(encrypted.begin(), encrypted.end());
    tampered[tampered.size() / 2] ^= 0xFF;

    byte_view decrypted = serverSession.Decrypt(tampered);
    CHECK(decrypted.empty());
}

TEST_CASE("crypto.session_aead_wrong_session")
{
    CryptoKeys clientKeys;
    CryptoKeys serverKeys;
    CryptoKeys rogueKeys;

    CryptoSession clientSession;
    CryptoSession rogueSession;

    clientSession.Init(clientKeys, serverKeys.RawPublicKey(), false, false);
    rogueSession.Init(rogueKeys, serverKeys.RawPublicKey(), false, true);

    std::vector<uint8_t> plaintext = {42};

    byte_view encrypted = clientSession.Encrypt(plaintext);
    REQUIRE_FALSE(encrypted.empty());

    // Wrong session keys should fail
    byte_view decrypted = rogueSession.Decrypt(encrypted);
    CHECK(decrypted.empty());
}
#endif
