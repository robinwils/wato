#include "test.hpp"

#include <core/net/net.hpp>
#include <core/snapshot.hpp>

TEST_CASE("net.serialize")
{
    BitOutputArchive outAr;
    auto*            ev = new NetworkRequest;

    std::uint64_t gameID =
        (static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count())
         << 32)
        | 42ul;

    ev->PlayerID = 42;
    ev->Type     = PacketType::ClientSync;
    ev->Tick     = 0;
    ev->Payload  = SyncPayload{.GameID = gameID, .State = GameState{.Tick = 12}};

    ev->Archive(outAr);

    BitInputArchive inAr(outAr.Data());
    auto*           ev2 = new NetworkRequest;

    ev2->Archive(inAr);

    CHECK_EQ(ev->PlayerID, ev2->PlayerID);
    CHECK_EQ(ev->Type, ev2->Type);
    CHECK_EQ(ev->Payload, ev2->Payload);
    delete ev;
    delete ev2;
}
