#pragma once

#include <sodium/crypto_kx.h>
#include <sodium/randombytes.h>
#include <sodium/utils.h>

#include <array>
#include <string>
#include <vector>

#include "core/types.hpp"

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

template <size_t N>
std::string KeyToB64(const std::array<unsigned char, N>& aKey)
{
    constexpr size_t kPublicBase64Len =
        sodium_base64_ENCODED_LEN(N, sodium_base64_VARIANT_ORIGINAL);
    std::string b64(kPublicBase64Len - 1, 0);

    if (nullptr
        == sodium_bin2base64(
            b64.data(),
            kPublicBase64Len,
            aKey.data(),
            aKey.size(),
            sodium_base64_VARIANT_ORIGINAL)) {
        return std::string{};
    }

    return b64;
}

// Holds only a public key. Used for sealed box encryption (crypto_box_seal).
class PublicKey
{
   public:
    using Key = std::array<unsigned char, crypto_kx_PUBLICKEYBYTES>;

    PublicKey() = default;
    explicit PublicKey(const Key& aKey) : mKey(aKey) {}

    byte_view Encrypt(byte_view aBytes);

    [[nodiscard]] const Key& Raw() const { return mKey; }

   private:
    Key                  mKey{};
    std::vector<uint8_t> mBuffer{};
};

// Full keypair (public + secret). Used for key exchange (crypto_kx) and sealed box decryption.
class CryptoKeys
{
   public:
    using Public = PublicKey::Key;
    using Secret = std::array<unsigned char, crypto_kx_SECRETKEYBYTES>;

    CryptoKeys() { crypto_kx_keypair(mPublic.data(), mSecret.data()); }

    [[nodiscard]] const std::string ExportPublicKey() const { return KeyToB64(mPublic); }

    [[nodiscard]] const Public RawPublicKey() const { return mPublic; }
    [[nodiscard]] const Secret RawSecretKey() const { return mSecret; }

    byte_view Decrypt(byte_view aBytes);

   private:
    Public mPublic{};
    Secret mSecret{};

    std::vector<uint8_t> mBuffer{};
};
