#pragma once

#include <string>

#include <chat/application/ports/uow/db_transaction.hpp>
#include <chat/domain/message.hpp>
#include <chat/domain/types/client_message_id.hpp>
#include <chat/domain/types/conversation_id.hpp>
#include <chat/domain/types/user_id.hpp>

namespace smirkly::chat::application::ports {
struct NewMessageData final {
  domain::ClientMessageId client_message_id;
  domain::ConversationId conversation_id;
  domain::UserId author_id;
  std::string text;
};

struct StoreMessageResult final {
  domain::Message message;
  bool was_created;
};

class MessageRepository {
 public:
  virtual ~MessageRepository() = default;

  virtual StoreMessageResult CreateOrGet(DbTransaction& transaction,
                                         const NewMessageData& data) = 0;
};
}  // namespace smirkly::chat::application::ports
