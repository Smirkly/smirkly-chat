#pragma once

#include <chat/application/ports/uow/db_transaction.hpp>
#include <chat/domain/types/conversation_id.hpp>
#include <chat/domain/types/user_id.hpp>

namespace smirkly::chat::application::ports {

class ConversationMemberRepository {
 public:
  virtual ~ConversationMemberRepository() = default;

  /// Returns whether the membership exists and keeps that observation stable
  /// until the transaction ends.
  virtual bool Exists(DbTransaction& transaction,
                      const domain::ConversationId& conversation_id,
                      const domain::UserId& user_id) = 0;
};

}  // namespace smirkly::chat::application::ports
