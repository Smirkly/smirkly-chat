#pragma once

#include <string>
#include <vector>

#include <chat/domain/message.hpp>
#include <chat/domain/types/client_message_id.hpp>
#include <chat/domain/types/conversation_id.hpp>
#include <chat/domain/types/user_id.hpp>

namespace smirkly::chat::application::contracts {
struct SendMessageCommand final {
  domain::UserId author_id;
  domain::ConversationId conversation_id;
  domain::ClientMessageId client_message_id;
  std::string text;
};

struct SendMessageResult final {
  domain::Message message;
  bool was_created;
  std::vector<domain::UserId> conversation_member_ids;
};
}  // namespace smirkly::chat::application::contracts
