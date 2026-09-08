#include <chat/application/usecases/send_message_use_case.hpp>

#include <cstddef>
#include <utility>

#include <chat/application/errors/send_message.hpp>

namespace {
constexpr std::size_t kMaxMessageTextSize = 4096;
}

namespace smirkly::chat::application::usecases {
SendMessageUseCase::SendMessageUseCase(ports::MessageRepository& messages)
    : messages_(messages) {}

contracts::SendMessageResult SendMessageUseCase::Execute(
    const contracts::SendMessageCommand& command) const {
  if (command.text.empty()) {
    throw errors::InvalidMessageText("the text is empty");
  }
  if (command.text.size() > kMaxMessageTextSize) {
    throw errors::MessageTooLong("the text is too long");
  }

  auto result =
      messages_.CreateOrGetIfMember(command.author_id, command.conversation_id,
                                    command.client_message_id, command.text);

  if (!result) {
    throw errors::ConversationAccessDenied("conversation access denied");
  }
  if (!result->was_created) {
    if ((result->message.GetConversationId() != command.conversation_id) ||
        (result->message.GetText() != command.text)) {
      throw errors::IdempotencyConflict("idempotency conflict");
    }
  }

  return contracts::SendMessageResult{.message = std::move(result->message),
                                      .was_created = result->was_created};
}

}  // namespace smirkly::chat::application::usecases
