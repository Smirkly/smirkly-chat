#pragma once

#include <memory>
#include <string_view>

#include <chat/application/ports/uow/db_transaction.hpp>

namespace smirkly::chat::application::ports {

class TransactionManager {
 public:
  virtual ~TransactionManager() noexcept = default;

  [[nodiscard]] virtual std::unique_ptr<DbTransaction> Begin(
      std::string_view transaction_name) = 0;
};

}  // namespace smirkly::chat::application::ports
