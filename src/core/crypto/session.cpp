#include "core/crypto/session.hpp"

byte_buffer CryptoSession::Encrypt(byte_view aBytes)
{
    unsigned char      nonce[MAX_NONCE_BYTES]{};
    unsigned long long cipherTextLen{};

    // TODO: counter/replay protection in nonce
    randombytes_buf(nonce, mAEAD->NonceBytes);

    // [nonce | ciphertext + tag]
    std::vector<uint8_t> buffer(mAEAD->NonceBytes + aBytes.size() + mAEAD->AuthTagBytes);

    std::memcpy(buffer.data(), nonce, mAEAD->NonceBytes);

    int ret = mAEAD->Encrypt(
        buffer.data() + mAEAD->NonceBytes,
        &cipherTextLen,
        aBytes.data(),
        aBytes.size(),
        nullptr,
        0,
        nullptr,
        nonce,
        mTX);

    return ret == 0 ? std::move(buffer) : byte_buffer{};
}

byte_buffer CryptoSession::Decrypt(byte_view aBytes)
{
    const auto minSize = mAEAD->NonceBytes + mAEAD->AuthTagBytes;
    if (aBytes.size() < minSize + 1) return {};

    byte_view nonce      = aBytes.subspan(0, mAEAD->NonceBytes);
    byte_view cipherText = aBytes.subspan(mAEAD->NonceBytes, aBytes.size() - mAEAD->NonceBytes);

    unsigned long long decryptedLen{};

    std::vector<uint8_t> buffer(cipherText.size() - mAEAD->AuthTagBytes);

    int ret = mAEAD->Decrypt(
        buffer.data(),
        &decryptedLen,
        nullptr,
        cipherText.data(),
        cipherText.size(),
        nullptr,
        0,
        nonce.data(),
        mRX);
    return ret == 0 ? std::move(buffer) : byte_buffer{};
}
