#include "core/net/enet_server.hpp"

#include <bx/spscqueue.h>
#include <enet.h>
#include <sodium/runtime.h>
#include <spdlog/spdlog.h>

#include <expected>
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
    auto* state       = new PeerState{.ID = 0};
    aEvent.peer->data = state;
    mLogger->info("peer connected, awaiting auth");
}

void ENetServer::OnReceive(ENetEvent& aEvent, byte_view aData)
{
    if (aData.empty()) {
        return;
    }

    auto* state = static_cast<PeerState*>(aEvent.peer->data);

    // Pre-session: sealed box from client handshake
    if (state && !state->SecureSession.Valid()) {
        byte_view decrypted = mKeys.Decrypt(aData);
        if (decrypted.empty()) {
            mLogger->error("Could not open sealed handshake");
            BitOutputArchive out;
            NetworkResponse  resp{
                 .Type     = PacketType::Nack,
                 .PlayerID = 0,
                 .Tick     = 0,
                 .Payload  = ErrorResponse{.Error = ServerError::HandshakeOpenSeal}};

            if (!resp.Archive(out)) {
                mLogger->error("Could not archive error response");
                return;
            }

            if (!ENetBase::Send(aEvent.peer, out.Bytes(), false)) {
                mLogger->error("Could send error response");
                return;
            }
            return;
        }
        aData = decrypted;
    }

    BitInputArchive archive(aData);
    auto*           ev = new NetworkRequest;

    if (!ev->Archive(archive)) {
        mLogger->critical("cannot decode packet");
        delete ev;
        return;
    }

    if (ev->Type == PacketType::Auth) {
        auto  auth = std::get<AuthRequest>(ev->Payload);
        auto* peer = aEvent.peer;

        mPBClient.RefreshToken(
            [peer,
             &chan    = mAuthResultChan,
             logger   = mLogger,
             hasAESNI = auth.HasAESNI,
             pub      = auth.PublicKey](std::expected<LoginResult, PBError> aResult) {
                if (aResult) {
                    auto playerID = IDFromHexString<PlayerID>(aResult->record.id);
                    if (playerID) {
                        chan.Send(new AuthResult{
                            .Peer        = peer,
                            .ID          = *playerID,
                            .AccountName = aResult->record.accountName,
                            .HasAESNI    = hasAESNI,
                            .PublicKey   = pub});
                        return;
                    }
                }
                logger->warn("auth verification failed: {}", aResult.error().Message);
            },
            auth.Token);
        delete ev;
        return;
    }

    if (!state || state->ID == 0) {
        mLogger->warn("dropping packet from unauthenticated peer");
        delete ev;
        return;
    }
    ev->PlayerID = state->ID;
    mReqChannel.Send(ev);
}

void ENetServer::ProcessAuthResults()
{
    mAuthResultChan.Drain([this](AuthResult* aResult) {
        if (aResult->Peer->state != ENET_PEER_STATE_CONNECTED) {
            mLogger->warn("peer disconnected before auth completed for player {}", aResult->ID);
            return;
        }

        auto* state   = static_cast<PeerState*>(aResult->Peer->data);
        state->ID     = aResult->ID;
        bool canAEGIS = aResult->HasAESNI && (sodium_runtime_has_aesni() != 0);

        state->SecureSession.Init(mKeys, aResult->PublicKey, canAEGIS, true);
        if (!state->SecureSession.Valid()) {
            mLogger->error("cannot compute server session keys");
            return;
        }
        state->AwaitingHandshake = true;

        mConnectedPeers[aResult->ID] = aResult->Peer;
        mAccountNames[aResult->ID]   = aResult->AccountName;
        mLogger->info(
            "player {} ({}) authenticated and registered",
            aResult->ID,
            aResult->AccountName);

        auto resp = NetworkResponse{
            .Type     = PacketType::Auth,
            .PlayerID = aResult->ID,
            .Tick     = 0,
            .Payload  = AuthResponse{.ID = aResult->ID, .HasAESNI = canAEGIS, .Success = true}};
        BitOutputArchive archive;
        resp.Archive(archive);
        Send(resp.PlayerID, archive.Bytes());
    });
}

void ENetServer::OnDisconnect(ENetEvent& aEvent)
{
    if (auto* state = static_cast<PeerState*>(aEvent.peer->data)) {
        mLogger->info("player {} disconnected", state->ID);
        mConnectedPeers.erase(state->ID);
        mAccountNames.erase(state->ID);
        delete state;
        aEvent.peer->data = nullptr;
    }
}

void ENetServer::OnDisconnectTimeout(ENetEvent& aEvent)
{
    if (auto* state = static_cast<PeerState*>(aEvent.peer->data)) {
        mLogger->warn("player {} timed out", state->ID);
        mConnectedPeers.erase(state->ID);
        mAccountNames.erase(state->ID);
        delete state;
        aEvent.peer->data = nullptr;
    }
}

void ENetServer::OnNone(ENetEvent&) {}
