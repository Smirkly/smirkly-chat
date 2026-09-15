#pragma once

#include <string>

#include <chat/api/v0/websocket/protocol/client_envelope.hpp>
#include <chat/application/usecases/send_message_use_case.hpp>
#include <chat/domain/types/user_id.hpp>

namespace smirkly::chat::api::v0::websocket {

class WebsocketDispatcher final {
 public:
  explicit WebsocketDispatcher(
      application::usecases::SendMessageUseCase& send_message);

  [[nodiscard]] std::string Dispatch(
      const protocol::ClientEnvelope& envelope,
      const domain::UserId& authenticated_user) const;

 private:
  application::usecases::SendMessageUseCase& send_message_;
};

}  // namespace smirkly::chat::api::v0::websocket
