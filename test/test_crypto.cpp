#include <doctest.h>

#include <core/crypto/key.hpp>
#include <core/crypto/session.hpp>

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
