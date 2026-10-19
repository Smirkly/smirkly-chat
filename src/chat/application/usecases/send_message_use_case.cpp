#include <chat/application/usecases/send_message_use_case.hpp>

#include <cstddef>
#include <string>
#include <utility>

#include <chat/application/errors/send_message.hpp>

namespace {
constexpr std::size_t kMaxMessageTextSize = 4096;
}

namespace smirkly::chat::application::usecases {
SendMessageUseCase::SendMessageUseCase(
    ports::TransactionManager& transactions,
    ports::ConversationMemberRepository& members,
    ports::MessageRepository& messages)
    : transactions_(transactions), members_(members), messages_(messages) {}

contracts::SendMessageResult SendMessageUseCase::Execute(
    const contracts::SendMessageCommand& command) const {
  if (command.text.empty()) {
    throw errors::InvalidMessageText("the text is empty");
  }
  if (command.text.find('\0') != std::string::npos) {
    throw errors::InvalidMessageText("the text contains a NUL byte");
  }
  if (command.text.size() > kMaxMessageTextSize) {
    throw errors::MessageTooLong("the text is too long");
  }

  auto transaction = transactions_.Begin("chat.message.send");
  if (!members_.Exists(*transaction, command.conversation_id,
                       command.author_id)) {
    throw errors::ConversationAccessDenied("conversation access denied");
  }

  const ports::NewMessageData data{
      .client_message_id = command.client_message_id,
      .conversation_id = command.conversation_id,
      .author_id = command.author_id,
      .text = command.text,
  };

  auto result = messages_.CreateOrGet(*transaction, data);

  if (!result.was_created &&
      ((result.message.GetConversationId() != command.conversation_id) ||
       (result.message.GetText() != command.text))) {
    throw errors::IdempotencyConflict("idempotency conflict");
  }

  transaction->Commit();

  return contracts::SendMessageResult{.message = std::move(result.message),
                                      .was_created = result.was_created};
}

}  // namespace smirkly::chat::application::usecases
