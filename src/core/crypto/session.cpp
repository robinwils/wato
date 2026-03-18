#include "core/crypto/session.hpp"

byte_view CryptoSession::Encrypt(byte_view aBytes)
{
    unsigned char      nonce[MAX_NONCE_BYTES]{};
    unsigned long long cipherTextLen{};

    // TODO: counter/replay protection in nonce
    randombytes_buf(nonce, mAEAD->NonceBytes);

    // [nonce | ciphertext + tag]
    mBuffer.resize(mAEAD->NonceBytes + aBytes.size() + mAEAD->AuthTagBytes);

    std::memcpy(mBuffer.data(), nonce, mAEAD->NonceBytes);

    int ret = mAEAD->Encrypt(
        mBuffer.data() + mAEAD->NonceBytes,
        &cipherTextLen,
        aBytes.data(),
        aBytes.size(),
        nullptr,
        0,
        nullptr,
        nonce,
        mTX);

    return ret == 0 ? mBuffer : byte_view{};
}

byte_view CryptoSession::Decrypt(byte_view aBytes)
{
    const auto minSize = mAEAD->NonceBytes + mAEAD->AuthTagBytes;
    if (aBytes.size() < minSize + 1) return {};

    byte_view nonce      = aBytes.subspan(0, mAEAD->NonceBytes);
    byte_view cipherText = aBytes.subspan(mAEAD->NonceBytes, aBytes.size() - mAEAD->NonceBytes);

    unsigned long long decryptedLen{};

    mBuffer.resize(cipherText.size() - mAEAD->AuthTagBytes);

    int ret = mAEAD->Decrypt(
        mBuffer.data(),
        &decryptedLen,
        nullptr,
        cipherText.data(),
        cipherText.size(),
        nullptr,
        0,
        nonce.data(),
        mRX);
    return ret == 0 ? mBuffer : byte_view{};
}
