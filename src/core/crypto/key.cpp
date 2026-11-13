#include "core/crypto/key.hpp"

#include <sodium/crypto_box.h>

byte_buffer PublicKey::Encrypt(byte_view aBytes)
{
    std::vector<uint8_t> buf(aBytes.size() + crypto_box_SEALBYTES);

    int ret = crypto_box_seal(buf.data(), aBytes.data(), aBytes.size(), mKey.data());

    return ret == 0 ? buf : byte_buffer{};
}

byte_buffer CryptoKeys::Decrypt(byte_view aBytes)
{
    std::vector<uint8_t> mBuffer(aBytes.size() - crypto_box_SEALBYTES);

    int ret = crypto_box_seal_open(
        mBuffer.data(),
        aBytes.data(),
        aBytes.size(),
        mPublic.data(),
        mSecret.data());
    return ret == 0 ? mBuffer : byte_buffer{};
}
