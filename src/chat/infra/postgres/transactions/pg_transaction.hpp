#pragma once

#include <userver/storages/postgres/transaction.hpp>

#include <chat/application/ports/uow/db_transaction.hpp>

namespace smirkly::chat::infra::postgres {

class PgTransactionManager;

class PgTransaction final : public application::ports::DbTransaction {
  friend class PgTransactionManager;

 public:
  PgTransaction(const PgTransaction&) = delete;
  PgTransaction& operator=(const PgTransaction&) = delete;
  PgTransaction(PgTransaction&&) = delete;
  PgTransaction& operator=(PgTransaction&&) = delete;

  ~PgTransaction() noexcept override = default;

  void Commit() override;

  USERVER_NAMESPACE::storages::postgres::Transaction& Native() noexcept;
  const USERVER_NAMESPACE::storages::postgres::Transaction& Native()
      const noexcept;

 private:
  explicit PgTransaction(
      USERVER_NAMESPACE::storages::postgres::Transaction transaction);

  USERVER_NAMESPACE::storages::postgres::Transaction transaction_;
  bool committed_{false};
};

}  // namespace smirkly::chat::infra::postgres
