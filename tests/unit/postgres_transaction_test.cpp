#include <chat/infra/postgres/transactions/pg_tx_cast.hpp>

#include <stdexcept>

#include <userver/utest/utest.hpp>

namespace smirkly::chat::infra::postgres {
namespace {

class OtherTransaction final : public application::ports::DbTransaction {
 public:
  void Commit() override {}
};

UTEST(PostgresTransaction, RejectsNonPostgresTransaction) {
  OtherTransaction transaction;

  EXPECT_THROW(AsPgTx(transaction, "test"), std::logic_error);
}

UTEST(PostgresTransaction, RejectsConstNonPostgresTransaction) {
  const OtherTransaction transaction;

  EXPECT_THROW(AsPgTx(transaction, "test"), std::logic_error);
}

}  // namespace
}  // namespace smirkly::chat::infra::postgres
