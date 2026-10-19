#pragma once

#include <chat/application/contracts/send_message.hpp>
#include <chat/application/ports/conversation_member_repository.hpp>
#include <chat/application/ports/message_repository.hpp>
#include <chat/application/ports/uow/transaction_manager.hpp>

namespace smirkly::chat::application::usecases {
class SendMessageUseCase final {
 public:
  SendMessageUseCase(ports::TransactionManager& transactions,
                     ports::ConversationMemberRepository& members,
                     ports::MessageRepository& messages);

  contracts::SendMessageResult Execute(
      const contracts::SendMessageCommand& command) const;

 private:
  ports::TransactionManager& transactions_;
  ports::ConversationMemberRepository& members_;
  ports::MessageRepository& messages_;
};
}  // namespace smirkly::chat::application::usecases
