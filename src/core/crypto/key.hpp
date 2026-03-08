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

