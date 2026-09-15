#include <chat/infra/postgres/mappers/message_mapper.hpp>

#include <utility>

namespace smirkly::chat::infra::postgres::mappers {

domain::Message ToDomain(types::MessageRow row) {
  return {
      domain::MessageId{row.id},
      domain::ClientMessageId{row.client_message_id},
      domain::ConversationId{row.conversation_id},
      domain::UserId{row.author_id},
      std::move(row.text),
      row.created_at.GetUnderlying(),
  };
}

}  // namespace smirkly::chat::infra::postgres::mappers
