#pragma once

#include <chat/application/ports/message_repository.hpp>

namespace smirkly::chat::infra::postgres {

class PostgresMessageRepository final
    : public application::ports::MessageRepository {
 public:
  application::ports::StoreMessageResult CreateOrGet(
      application::ports::DbTransaction& transaction,
      const application::ports::NewMessageData& data) override;
};
}  // namespace smirkly::chat::infra::postgres
