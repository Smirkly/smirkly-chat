#pragma once

#include <memory>
#include <string_view>

#include <userver/storages/postgres/cluster.hpp>

#include <chat/application/ports/uow/transaction_manager.hpp>

namespace smirkly::chat::infra::postgres {

class PgTransactionManager final
    : public application::ports::TransactionManager {
 public:
  explicit PgTransactionManager(
      USERVER_NAMESPACE::storages::postgres::ClusterPtr cluster);

  [[nodiscard]] std::unique_ptr<application::ports::DbTransaction> Begin(
      std::string_view transaction_name) override;

 private:
  USERVER_NAMESPACE::storages::postgres::ClusterPtr cluster_;
};

}  // namespace smirkly::chat::infra::postgres
