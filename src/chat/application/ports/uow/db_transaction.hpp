#pragma once

namespace smirkly::chat::application::ports {

class DbTransaction {
 public:
  virtual ~DbTransaction() noexcept = default;

  virtual void Commit() = 0;
};

}  // namespace smirkly::chat::application::ports
