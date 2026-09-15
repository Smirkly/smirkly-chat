#include <chat/infra/postgres/transactions/pg_transaction.hpp>

#include <stdexcept>
#include <utility>

namespace smirkly::chat::infra::postgres {

PgTransaction::PgTransaction(
    USERVER_NAMESPACE::storages::postgres::Transaction transaction)
    : transaction_(std::move(transaction)) {}

void PgTransaction::Commit() {
  if (committed_) {
    throw std::logic_error("PostgreSQL transaction is already committed");
  }

  transaction_.Commit();
  committed_ = true;
}

USERVER_NAMESPACE::storages::postgres::Transaction&
PgTransaction::Native() noexcept {
  return transaction_;
}

const USERVER_NAMESPACE::storages::postgres::Transaction&
PgTransaction::Native() const noexcept {
  return transaction_;
}

}  // namespace smirkly::chat::infra::postgres
