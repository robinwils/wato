#define ENET_IMPLEMENTATION
#define ENET_FEATURE_ADDRESS_MAPPING

#include "core/net/enet_base.hpp"

#include <enet.h>
#include <sodium.h>
#include <spdlog/spdlog.h>

#include <stdexcept>

#include "core/crypto/key.hpp"
#include "core/net/net.hpp"
#include "core/sys/log.hpp"

inline constexpr const std::string peerDataStr(const ENetEvent& aEvent)
{
    std::string peerData = "(unauthenticated)";
    if (aEvent.peer && aEvent.peer->data) {
        peerData = "player " + std::to_string(static_cast<PeerState*>(aEvent.peer->data)->ID);
    }

    return peerData;
}

ENetBase::~ENetBase() { enet_deinitialize(); }

void ENetBase::Init()
{
    if (enet_initialize() != 0) {
        throw std::runtime_error("failed to initialize Enet");
    }
}

bool ENetBase::Send(ENetPeer* aPeer, const std::span<const uint8_t> aData)
{
    if (aPeer == nullptr) {
        mLogger->debug("client peer not initialized");
        return false;
    }

    auto* state = static_cast<PeerState*>(aPeer->data);
    if (!state || !state->SecureSession.Valid()) {
        mLogger->error("Peer not initialized or session not established");
        return false;
    }

    byte_view enc = state->SecureSession.Encrypt(aData);
    if (enc.empty()) {
        mLogger->error("Could not encrypt peer data");
        return false;
    }

    auto data = byte_buffer(enc.begin(), enc.end());

    ENetPacket* packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);

    if (-1 == enet_peer_send(aPeer, 0, packet)) {
        enet_packet_destroy(packet);
        return false;
    }

    enet_host_flush(mHost.get());
    return true;
}

void ENetBase::Poll()
{
    // TODO: not propagated if thrown in thread
    if (!mHost) {
        throw std::runtime_error("host is not initialized");
    }
    ENetEvent event;

    /* Wait up to x milliseconds for an event. (WARNING: blocking) */
    while (enet_host_service(mHost.get(), &event, 5) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (event.peer) {
                    mLogger->info("New {}.\n", *event.peer);
                }
                /* Store any relevant client information here. */
                OnConnect(event);
                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                std::string userData   = "(null)";
                std::size_t packetSize = 0;
                if (event.packet && event.packet->data) {
                    userData = std::string(
                        reinterpret_cast<char*>(event.packet->data),
                        event.packet->dataLength);
                    packetSize = event.packet->dataLength;
                }

                mLogger->trace(
                    "A packet of length {} was received from {} with data "
                    "{} on "
                    "channel {}.",
                    packetSize,
                    *event.peer,
                    peerDataStr(event),
                    event.channelID);

                auto* state = static_cast<PeerState*>(event.peer->data);
                if (!state) {
                    mLogger->error("Peer not initialized");
                    enet_packet_destroy(event.packet);
                    break;
                }

                if (state->SecureSession.Valid()) {
                    byte_view decrypted = state->SecureSession.Decrypt(
                        {event.packet->data, event.packet->dataLength});
                    if (decrypted.empty()) {
                        mLogger->error("Could not decrypt data");
                        enet_packet_destroy(event.packet);
                        break;
                    }

                    OnReceive(event, decrypted);
                } else {
                    OnReceive(event, {event.packet->data, event.packet->dataLength});
                }

                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                mLogger->info("{} disconnected.\n", peerDataStr(event));
                OnDisconnect(event);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                mLogger->info("{} disconnected due to timeout.\n", peerDataStr(event));
                OnDisconnectTimeout(event);
                break;
            }
            case ENET_EVENT_TYPE_NONE:
                OnNone(event);
                break;
        }
    }
}
