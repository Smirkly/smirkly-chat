#include <chat/infra/postgres/postgres_conversation_member_repository.hpp>

#include <smirkly::chat/sql_queries.hpp>
#include <userver/storages/postgres/io/uuid.hpp>

#include <chat/infra/postgres/transactions/pg_tx_cast.hpp>

namespace smirkly::chat::infra::postgres {
bool PostgresConversationMemberRepository::Exists(
    application::ports::DbTransaction& transaction,
    const domain::ConversationId& conversation_id,
    const domain::UserId& user_id) {
  auto& pg_tx =
      AsPgTx(transaction, "PostgresConversationMemberRepository::Exists");

  const auto res = pg_tx.Native().Execute(
      sql::kConversationMembersLock, conversation_id.Value(), user_id.Value());

  return !res.IsEmpty();
}

std::vector<domain::UserId> PostgresConversationMemberRepository::ListUserIds(
    application::ports::DbTransaction& transaction,
    const domain::ConversationId& conversation_id) {
  auto& pg_tx =
      AsPgTx(transaction, "PostgresConversationMemberRepository::ListUserIds");
  const auto rows = pg_tx.Native().Execute(sql::kConversationMembersList,
                                           conversation_id.Value());
  std::vector<domain::UserId> result;
  result.reserve(rows.Size());
  for (const auto& row : rows) {
    result.emplace_back(row["user_id"].As<boost::uuids::uuid>());
  }
  return result;
}

}  // namespace smirkly::chat::infra::postgres
