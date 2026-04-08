#include <doctest.h>

#include <core/types.hpp>

TEST_CASE("types.ToGameID")
{
    SUBCASE("types.ToGameID.zero")
    {
        auto result = IDToHexString<GameInstanceID>(0);
        REQUIRE(result.has_value());
        CHECK(result.value() == "0");
    }

    SUBCASE("types.ToGameID.deadbeef")
    {
        auto result = IDToHexString<GameInstanceID>(0xDEADBEEF);
        REQUIRE(result.has_value());
        CHECK(result.value() == "deadbeef");
    }

    SUBCASE("types.ToGameID.uint64_max")
    {
        auto result = IDToHexString<GameInstanceID>(UINT64_MAX);
        REQUIRE(result.has_value());
        CHECK(result.value() == "ffffffffffffffff");
    }

    SUBCASE("types.ToPlayerID.zero")
    {
        auto result = IDToHexString<PlayerID>(0);
        REQUIRE(result.has_value());
        CHECK(result.value() == "0");
    }

    SUBCASE("types.ToPlayerID.deadbeef")
    {
        auto result = IDToHexString<PlayerID>(0xDEADBEEF);
        REQUIRE(result.has_value());
        CHECK(result.value() == "deadbeef");
    }

    SUBCASE("types.ToPlayerID.uint32_max")
    {
        auto result = IDToHexString<PlayerID>(UINT32_MAX);
        REQUIRE(result.has_value());
        CHECK(result.value() == "ffffffff");
    }
}

TEST_CASE("types.FromGameID")
{
    SUBCASE("types.FromGameID.empty")
    {
        auto result = IDFromHexString<GameInstanceID>("");
        CHECK_FALSE(result.has_value());
        CHECK(result.error() == std::errc::invalid_argument);
    }

    SUBCASE("types.FromGameID.zero")
    {
        auto result = IDFromHexString<GameInstanceID>("0");
        REQUIRE(result.has_value());
        CHECK(result.value() == 0);
    }

    SUBCASE("types.FromGameID.deadbeef")
    {
        auto result = IDFromHexString<GameInstanceID>("deadbeef");
        REQUIRE(result.has_value());
        CHECK(result.value() == 0xDEADBEEF);
    }

    SUBCASE("types.FromGameID.DEADBEEF")
    {
        auto result = IDFromHexString<GameInstanceID>("DEADBEEF");
        REQUIRE(result.has_value());
        CHECK(result.value() == 0xDEADBEEF);
    }

    SUBCASE("types.FromGameID.invalid")
    {
        auto result = IDFromHexString<GameInstanceID>("xyz");
        CHECK_FALSE(result.has_value());
    }

    SUBCASE("types.FromPlayerID.empty")
    {
        auto result = IDFromHexString<PlayerID>("");
        CHECK_FALSE(result.has_value());
        CHECK(result.error() == std::errc::invalid_argument);
    }

    SUBCASE("types.FromPlayerID.value")
    {
        auto result = IDFromHexString<PlayerID>("ff");
        REQUIRE(result.has_value());
        CHECK(result.value() == 255);
    }

    SUBCASE("types.FromPlayerID.max_uint32")
    {
        auto result = IDFromHexString<PlayerID>("ffffffff");
        REQUIRE(result.has_value());
        CHECK(result.value() == UINT32_MAX);
    }

    SUBCASE("types.FromPlayerID.invalid")
    {
        auto result = IDFromHexString<PlayerID>("zzzz");
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("types.ToGameID.round_trip")
{
    constexpr GameInstanceID kOriginal = 0x123456789ABCDEF0;
    auto                     hexStr    = IDToHexString<GameInstanceID>(kOriginal);
    REQUIRE(hexStr.has_value());
    auto parsed = IDFromHexString<GameInstanceID>(hexStr.value());
    REQUIRE(parsed.has_value());
    CHECK(parsed.value() == kOriginal);
}

TEST_CASE("types.PlayerID.round_trip")
{
    const std::string kOriginal = "5ac6fc50";
    auto              pID       = IDFromHexString<PlayerID>(kOriginal);
    REQUIRE(pID.has_value());
    auto parsed = IDToHexString<PlayerID>(pID.value());
    REQUIRE(parsed.has_value());
    CHECK(parsed.value() == kOriginal);
}
