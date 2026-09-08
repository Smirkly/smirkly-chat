#pragma once

#include <optional>
#include <string>

#include <chat/domain/message.hpp>
#include <chat/domain/types/client_message_id.hpp>
#include <chat/domain/types/conversation_id.hpp>
#include <chat/domain/types/user_id.hpp>

namespace smirkly::chat::application::ports {
struct StoreMessageResult final {
  domain::Message message;
  bool was_created;
};

class MessageRepository {
 public:
  virtual ~MessageRepository() = default;

  /// Atomically checks membership and creates the message, or returns
  /// the existing message for the same (author_id, client_message_id).
  virtual std::optional<StoreMessageResult> CreateOrGetIfMember(
      const domain::UserId& author_id,
      const domain::ConversationId& conversation_id,
      const domain::ClientMessageId& client_message_id,
      const std::string& text) = 0;
};
}  // namespace smirkly::chat::application::ports