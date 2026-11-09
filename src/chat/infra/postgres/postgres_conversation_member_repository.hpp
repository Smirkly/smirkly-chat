#pragma once

#include <vector>

#include <chat/application/ports/conversation_member_repository.hpp>

namespace smirkly::chat::infra::postgres {

class PostgresConversationMemberRepository final
    : public application::ports::ConversationMemberRepository {
 public:
  bool Exists(application::ports::DbTransaction& transaction,
              const domain::ConversationId& conversation_id,
              const domain::UserId& user_id) override;

  std::vector<domain::UserId> ListUserIds(
      application::ports::DbTransaction& transaction,
      const domain::ConversationId& conversation_id) override;
};

}  // namespace smirkly::chat::infra::postgres
