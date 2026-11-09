#pragma once

#include <string>
#include <string_view>

#include <chat/api/v0/websocket/protocol/client_envelope.hpp>
#include <chat/application/delivery/connection_registry.hpp>
#include <chat/application/usecases/send_message_use_case.hpp>
#include <chat/domain/types/user_id.hpp>

namespace smirkly::chat::api::v0::websocket {

class WebsocketDispatcher final {
 public:
  WebsocketDispatcher(
      application::usecases::SendMessageUseCase& send_message,
      application::delivery::ConnectionRegistry& connection_registry);

  [[nodiscard]] std::string Dispatch(
      const protocol::ClientEnvelope& envelope,
      const domain::UserId& authenticated_user,
      std::string_view connection_id) const;

 private:
  application::usecases::SendMessageUseCase& send_message_;
  application::delivery::ConnectionRegistry& connection_registry_;
};

}  // namespace smirkly::chat::api::v0::websocket
