#pragma once

#include <sodium/crypto_kx.h>
#include <sodium/randombytes.h>
#include <sodium/utils.h>

#include <array>
#include <string>
#include <vector>

#include "core/types.hpp"

// Holds only a public key. Used for sealed box encryption (crypto_box_seal).
class PublicKey
{
   public:
    using Key = std::array<unsigned char, crypto_kx_PUBLICKEYBYTES>;

    PublicKey() = default;
    explicit PublicKey(const Key& aKey) : mKey(aKey) {}

    byte_view Encrypt(byte_view aBytes);

    const Key& Raw() const { return mKey; }

   private:
    Key                  mKey{};
    std::vector<uint8_t> mBuffer{};
};

// Full keypair (public + secret). Used for key exchange (crypto_kx) and sealed box decryption.
class CryptoKeys
{
   public:
    static constexpr size_t kPublicBase64Len =
        sodium_base64_ENCODED_LEN(crypto_kx_PUBLICKEYBYTES, sodium_base64_VARIANT_ORIGINAL);
    using Public = PublicKey::Key;
    using Secret = std::array<unsigned char, crypto_kx_SECRETKEYBYTES>;

    CryptoKeys() { crypto_kx_keypair(mPublic.data(), mSecret.data()); }

    const std::string ExportPublicKey() const
    {
        std::string b64(kPublicBase64Len - 1, 0);

        if (nullptr
            == sodium_bin2base64(
                b64.data(),
                kPublicBase64Len,
                mPublic.data(),
                mPublic.size(),
                sodium_base64_VARIANT_ORIGINAL)) {
            return std::string{};
        }

        return b64;
    }

    const Public RawPublicKey() const { return mPublic; }
    const Secret RawSecretKey() const { return mSecret; }

    byte_view Decrypt(byte_view aBytes);

   private:
    Public mPublic{};
    Secret mSecret{};

    std::vector<uint8_t> mBuffer{};
};

template <size_t N>
std::array<unsigned char, N> KeyFromB64(const std::string& aB64Key)
{
    std::array<unsigned char, N> key{};
    size_t                       decLen{};

    if (0
        != sodium_base642bin(
            key.data(),
            N,
            aB64Key.data(),
            aB64Key.size(),
            nullptr,
            &decLen,
            nullptr,
            sodium_base64_VARIANT_ORIGINAL)) {
        return {};
    }

    return key;
}

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>

TEST_CASE("crypto.sealed_box_round_trip")
{
    CryptoKeys keys;
    PublicKey  pub(keys.RawPublicKey());

    std::vector<uint8_t> plaintext = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"

    byte_view encrypted = pub.Encrypt(plaintext);
    REQUIRE_FALSE(encrypted.empty());
    CHECK(encrypted.size() > plaintext.size());

    byte_view decrypted = keys.Decrypt(encrypted);
    REQUIRE_FALSE(decrypted.empty());
    CHECK(std::vector<uint8_t>(decrypted.begin(), decrypted.end()) == plaintext);
}

TEST_CASE("crypto.sealed_box_wrong_key")
{
    CryptoKeys keysA;
    CryptoKeys keysB;
    PublicKey  pub(keysA.RawPublicKey());

    std::vector<uint8_t> plaintext = {1, 2, 3, 4};

    byte_view encrypted = pub.Encrypt(plaintext);
    REQUIRE_FALSE(encrypted.empty());

    // Decrypt with wrong keypair should fail
    byte_view decrypted = keysB.Decrypt(encrypted);
    CHECK(decrypted.empty());
}

TEST_CASE("crypto.sealed_box_tampered")
{
    CryptoKeys keys;
    PublicKey  pub(keys.RawPublicKey());

    std::vector<uint8_t> plaintext = {10, 20, 30};

    byte_view encrypted = pub.Encrypt(plaintext);
    REQUIRE_FALSE(encrypted.empty());

    // Tamper with ciphertext
    std::vector<uint8_t> tampered(encrypted.begin(), encrypted.end());
    tampered[tampered.size() / 2] ^= 0xFF;

    byte_view decrypted = keys.Decrypt(tampered);
    CHECK(decrypted.empty());
}

TEST_CASE("crypto.base64_round_trip")
{
    CryptoKeys keys;

    std::string b64 = keys.ExportPublicKey();
    REQUIRE_FALSE(b64.empty());
    CHECK(b64.size() == CryptoKeys::kPublicBase64Len - 1);

    auto decoded = KeyFromB64<crypto_kx_PUBLICKEYBYTES>(b64);
    CHECK(decoded == keys.RawPublicKey());
}

TEST_CASE("crypto.base64_invalid_input")
{
    auto               decoded = KeyFromB64<crypto_kx_PUBLICKEYBYTES>("not_valid_base64!!!");
    CryptoKeys::Public zeroed{};
    CHECK(decoded == zeroed);
}

TEST_CASE("crypto.base64_empty_input")
{
    auto               decoded = KeyFromB64<crypto_kx_PUBLICKEYBYTES>("");
    CryptoKeys::Public zeroed{};
    CHECK(decoded == zeroed);
}

TEST_CASE("crypto.base64_sealed_box_integration")
{
    // Simulate the full PocketBase flow: server exports b64, client decodes and uses it
    CryptoKeys serverKeys;

    std::string b64 = serverKeys.ExportPublicKey();
    REQUIRE_FALSE(b64.empty());

    auto      serverPKRaw = KeyFromB64<crypto_kx_PUBLICKEYBYTES>(b64);
    PublicKey serverPK(serverPKRaw);

    std::vector<uint8_t> plaintext = {0xAB, 0xCD, 0xEF};

    byte_view encrypted = serverPK.Encrypt(plaintext);
    REQUIRE_FALSE(encrypted.empty());

    byte_view decrypted = serverKeys.Decrypt(encrypted);
    REQUIRE_FALSE(decrypted.empty());
    CHECK(std::vector<uint8_t>(decrypted.begin(), decrypted.end()) == plaintext);
}
#endif
