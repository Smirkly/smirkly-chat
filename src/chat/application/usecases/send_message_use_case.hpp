#pragma once

#include <chat/application/contracts/send_message.hpp>
#include <chat/application/ports/message_repository.hpp>

namespace smirkly::chat::application::usecases {
class SendMessageUseCase final {
 public:
  explicit SendMessageUseCase(ports::MessageRepository& messages);

  contracts::SendMessageResult Execute(
      const contracts::SendMessageCommand& command) const;

 private:
  ports::MessageRepository& messages_;
};
}  // namespace smirkly::chat::application::usecases