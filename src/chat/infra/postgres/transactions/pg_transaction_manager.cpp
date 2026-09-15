#include <chat/infra/postgres/transactions/pg_transaction_manager.hpp>

#include <memory>
#include <string>
#include <utility>

#include <userver/storages/postgres/cluster_types.hpp>

#include <chat/infra/postgres/transactions/pg_transaction.hpp>

namespace smirkly::chat::infra::postgres {

PgTransactionManager::PgTransactionManager(
    USERVER_NAMESPACE::storages::postgres::ClusterPtr cluster)
    : cluster_(std::move(cluster)) {}

std::unique_ptr<application::ports::DbTransaction> PgTransactionManager::Begin(
    std::string_view transaction_name) {
  auto transaction = cluster_->Begin(
      std::string{transaction_name},
      USERVER_NAMESPACE::storages::postgres::ClusterHostType::kMaster, {});

  return std::unique_ptr<application::ports::DbTransaction>{
      new PgTransaction{std::move(transaction)}};
}

}  // namespace smirkly::chat::infra::postgres
