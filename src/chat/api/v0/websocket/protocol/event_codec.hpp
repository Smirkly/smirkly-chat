#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <chat/api/v0/websocket/protocol/client_envelope.hpp>
#include <chat/api/v0/websocket/protocol/error.hpp>
#include <chat/api/v0/websocket/protocol/server_envelope.hpp>

namespace smirkly::chat::api::v0::websocket::protocol {

ClientEnvelope DecodeClientEnvelope(std::string_view data);

std::string EncodeServerEnvelope(const ServerEnvelope& envelope);

std::string EncodeError(
    const ProtocolError& error,
    std::optional<std::string_view> request_id = std::nullopt);
}  // namespace smirkly::chat::api::v0::websocket::protocol
