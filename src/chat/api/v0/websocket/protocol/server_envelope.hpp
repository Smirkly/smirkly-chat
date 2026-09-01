#pragma once

#include <optional>
#include <string>

#include <userver/formats/json/value.hpp>

namespace smirkly::chat::api::v0::websocket::protocol {

struct ServerEnvelope final {
  std::string type;
  std::optional<std::string> request_id;
  userver::formats::json::Value payload;
};
}  // namespace smirkly::chat::api::v0::websocket::protocol
