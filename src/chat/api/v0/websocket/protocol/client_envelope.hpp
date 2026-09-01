#pragma once

#include <string>

#include <userver/formats/json/value.hpp>

namespace smirkly::chat::api::v0::websocket::protocol {

struct ClientEnvelope final {
  std::string type;
  std::string request_id;
  userver::formats::json::Value payload;
};
}  // namespace smirkly::chat::api::v0::websocket::protocol
