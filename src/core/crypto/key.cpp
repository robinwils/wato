#include "core/crypto/key.hpp"

#include <sodium/crypto_box.h>

byte_view PublicKey::Encrypt(byte_view aBytes)
{
    mBuffer.resize(aBytes.size() + crypto_box_SEALBYTES);

    int ret = crypto_box_seal(mBuffer.data(), aBytes.data(), aBytes.size(), mKey.data());

    return ret == 0 ? mBuffer : byte_view{};
}

byte_view CryptoKeys::Decrypt(byte_view aBytes)
{
    mBuffer.resize(aBytes.size() - crypto_box_SEALBYTES);

    int ret = crypto_box_seal_open(
        mBuffer.data(),
        aBytes.data(),
        aBytes.size(),
        mPublic.data(),
        mSecret.data());
    return ret == 0 ? mBuffer : byte_view{};
}
