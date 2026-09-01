#pragma once

#include <stdexcept>
#include <string>

namespace smirkly::chat::api::v0::websocket::protocol {

struct ProtocolError final {
  std::string code;
  std::string message;
};

class InvalidEnvelope final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};
}  // namespace smirkly::chat::api::v0::websocket::protocol
