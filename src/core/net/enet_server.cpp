#include "core/net/enet_server.hpp"

#include <bx/spscqueue.h>
#include <enet.h>
#include <spdlog/spdlog.h>

#include <span>
#include <stdexcept>

#include "components/player.hpp"
#include "core/net/net.hpp"
#include "core/net/pocketbase.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"

void ENetServer::Init()
{
    ENetBase::Init();

    const std::size_t pos  = mServerAddr.find(':');
    enet_uint16       port = 7777;
    std::string       host = mServerAddr;

    if (pos != std::string::npos) {
        host = mServerAddr.substr(0, pos);
    }

    // Bind the server to the default localhost.
    ENetAddress address = {.host = ENET_HOST_ANY, .port = port, .sin6_scope_id = 0};

    if (host != "any") {
        if (-1 == enet_address_set_host(&address, host.c_str())) {
            mLogger->error(
                "An error occurred while trying to set ENet server host on '{}'",
                host.c_str());
            return;
        }
    }

    mHost = enet_host_ptr{enet_host_create(&address, 128, 2, 0, 0)};

    if (!mHost) {
        mLogger->error("An error occurred while trying to create an ENet server host.");
        return;
    }
    mLogger->info("created ENet server at {}:{}", host, port);
}

void ENetServer::OnConnect(ENetEvent& aEvent)
{
    BX_UNUSED(aEvent);
    mLogger->info("peer connected, awaiting auth");
}

void ENetServer::OnReceive(ENetEvent& aEvent)
{
    BitInputArchive archive(aEvent.packet->data, aEvent.packet->dataLength);
    auto*           ev = new NetworkRequest;

    if (!ev->Archive(archive)) {
        spdlog::critical("cannot decode packet");
        delete ev;
        return;
    }

    if (ev->Type == PacketType::Auth) {
        auto  token = std::get<AuthRequest>(ev->Payload).Token;
        auto* peer  = aEvent.peer;

        mPBClient.RefreshToken(
            [peer, &chan = mAuthResultChan, logger = mLogger](
                const std::optional<LoginResult>& aResult,
                const std::string&                aError) {
                if (aResult) {
                    auto playerID = PlayerIDFromHexString(aResult->record.id);
                    if (playerID) {
                        chan.Send(new AuthResult{
                            peer, *playerID, aResult->record.accountName});
                        return;
                    }
                }
                logger->warn("auth verification failed: {}", aError);
            },
            token);
        delete ev;
        return;
    }

    mReqChannel.Send(ev);
}

void ENetServer::ProcessAuthResults()
{
    mAuthResultChan.Drain([this](AuthResult* aResult) {
        mConnectedPeers[aResult->ID] = aResult->Peer;
        mAccountNames[aResult->ID]   = aResult->AccountName;
        mLogger->info(
            "player {} ({}) authenticated and registered", aResult->ID, aResult->AccountName);

        auto resp = NetworkResponse{
            .Type     = PacketType::Auth,
            .PlayerID = aResult->ID,
            .Tick     = 0,
            .Payload  = AuthResponse{.ID = aResult->ID, .Success = true}};
        BitOutputArchive archive;
        resp.Archive(archive);
        ENetBase::Send(aResult->Peer, archive.Bytes());
    });
}

void ENetServer::OnDisconnect(ENetEvent&) {}

void ENetServer::OnDisconnectTimeout(ENetEvent&) {}

void ENetServer::OnNone(ENetEvent&) {}
