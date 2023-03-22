// This file Copyright © 2007-2023 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#ifndef __TRANSMISSION__
#error only libtransmission should #include this header.
#endif

#include <atomic>
#include <cstdint> // int8_t
#include <cstddef> // size_t
#include <ctime> // time_t
#include <memory>
#include <utility>

#include "peer-common.h"
#include "torrent.h"

class tr_peer;
class tr_peerIo;
struct tr_address;

/**
 * @addtogroup peers Peers
 * @{
 */

class tr_peerMsgs : public tr_peer
{
public:
    tr_peerMsgs(
        tr_torrent const* tor,
        peer_atom* atom_in,
        bool connection_is_encrypted,
        bool connection_is_incoming,
        bool connection_is_utp)
        : tr_peer{ tor, atom_in }
        , connection_is_encrypted_{ connection_is_encrypted }
        , connection_is_incoming_{ connection_is_incoming }
        , connection_is_utp_{ connection_is_utp }
    {
        ++n_peers;
    }

    virtual ~tr_peerMsgs() override;

    [[nodiscard]] static size_t size() noexcept
    {
        return n_peers.load();
    }

    [[nodiscard]] constexpr auto client_is_choked() const noexcept
    {
        return client_is_choked_;
    }

    [[nodiscard]] constexpr auto client_is_interested() const noexcept
    {
        return client_is_interested_;
    }

    [[nodiscard]] constexpr auto peer_is_choked() const noexcept
    {
        return peer_is_choked_;
    }

    [[nodiscard]] constexpr auto peer_is_interested() const noexcept
    {
        return peer_is_interested_;
    }

    [[nodiscard]] constexpr auto is_encrypted() const noexcept
    {
        return connection_is_encrypted_;
    }

    [[nodiscard]] constexpr auto is_incoming_connection() const noexcept
    {
        return connection_is_incoming_;
    }

    [[nodiscard]] constexpr auto is_utp_connection() const noexcept
    {
        return connection_is_utp_;
    }

    [[nodiscard]] virtual bool is_active(tr_direction direction) const = 0;
    virtual void update_active(tr_direction direction) = 0;

    [[nodiscard]] virtual std::pair<tr_address, tr_port> socketAddress() const = 0;

    virtual void cancel_block_request(tr_block_index_t block) = 0;

    virtual void set_choke(bool peer_is_choked) = 0;
    virtual void set_interested(bool client_is_interested) = 0;

    virtual void pulse() = 0;

    virtual void onTorrentGotMetainfo() = 0;

    virtual void on_piece_completed(tr_piece_index_t) = 0;

    /// The client name. This is the app name derived from the `v` string in LTEP's handshake dictionary
    tr_interned_string client;

protected:
    constexpr void set_client_choked(bool val) noexcept
    {
        client_is_choked_ = val;
    }

    constexpr void set_client_interested(bool val) noexcept
    {
        client_is_interested_ = val;
    }

    constexpr void set_peer_choked(bool val) noexcept
    {
        peer_is_choked_ = val;
    }

    constexpr void set_peer_interested(bool val) noexcept
    {
        peer_is_interested_ = val;
    }

private:
    static inline auto n_peers = std::atomic<size_t>{};

    // whether or not the peer is choking us.
    bool client_is_choked_ = true;

    // whether or not we've indicated to the peer that we would download from them if unchoked
    bool client_is_interested_ = false;

    // whether or not we've choked this peer
    bool peer_is_choked_ = true;

    // whether or not the peer has indicated it will download from us
    bool peer_is_interested_ = false;

    bool const connection_is_encrypted_;
    bool const connection_is_incoming_;
    bool const connection_is_utp_;
};

tr_peerMsgs* tr_peerMsgsNew(
    tr_torrent* torrent,
    peer_atom* atom,
    std::shared_ptr<tr_peerIo> io,
    tr_peer_callback callback,
    void* callback_data);

/* @} */
