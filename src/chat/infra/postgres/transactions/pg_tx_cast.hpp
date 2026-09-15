#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include <chat/application/ports/uow/db_transaction.hpp>
#include <chat/infra/postgres/transactions/pg_transaction.hpp>

namespace smirkly::chat::infra::postgres {

inline PgTransaction& AsPgTx(application::ports::DbTransaction& transaction,
                             std::string_view where) {
  auto* postgres_transaction = dynamic_cast<PgTransaction*>(&transaction);
  if (!postgres_transaction) {
    throw std::logic_error(std::string{where} +
                           ": transaction is not PostgreSQL");
  }
  return *postgres_transaction;
}

inline const PgTransaction& AsPgTx(
    const application::ports::DbTransaction& transaction,
    std::string_view where) {
  const auto* postgres_transaction =
      dynamic_cast<const PgTransaction*>(&transaction);
  if (!postgres_transaction) {
    throw std::logic_error(std::string{where} +
                           ": transaction is not PostgreSQL");
  }
  return *postgres_transaction;
}

}  // namespace smirkly::chat::infra::postgres
