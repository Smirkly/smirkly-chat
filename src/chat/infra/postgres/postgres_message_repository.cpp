#include <chat/infra/postgres/postgres_message_repository.hpp>

#include <stdexcept>
#include <utility>

#include <smirkly::chat/sql_queries.hpp>
#include <userver/storages/postgres/io/row_types.hpp>
#include <userver/storages/postgres/io/uuid.hpp>

#include <chat/infra/postgres/mappers/message_mapper.hpp>
#include <chat/infra/postgres/transactions/pg_tx_cast.hpp>
#include <chat/infra/postgres/types/message_row.hpp>

namespace smirkly::chat::infra::postgres {
application::ports::StoreMessageResult PostgresMessageRepository::CreateOrGet(
    application::ports::DbTransaction& transaction,
    const application::ports::NewMessageData& data) {
  auto& pg_tx = AsPgTx(transaction, "PostgresMessageRepository::CreateOrGet");

  auto res = pg_tx.Native().Execute(
      sql::kMessagesInsert, data.client_message_id.Value(),
      data.conversation_id.Value(), data.author_id.Value(), data.text);

  const bool was_created = !res.IsEmpty();
  if (!was_created) {
    res = pg_tx.Native().Execute(sql::kMessagesFindByIdempotencyKey,
                                 data.author_id.Value(),
                                 data.client_message_id.Value());

    if (res.IsEmpty()) {
      throw std::logic_error(
          "PostgresMessageRepository::CreateOrGet: insert conflicted, but "
          "existing message was not found");
    }
  }

  auto row =
      res.AsSingleRow<types::MessageRow>(userver::storages::postgres::kRowTag);
  auto message = mappers::ToDomain(std::move(row));
  return application::ports::StoreMessageResult{.message = std::move(message),
                                                .was_created = was_created};
}
}  // namespace smirkly::chat::infra::postgres
