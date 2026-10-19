#pragma once

#include <boost/uuid/uuid.hpp>

namespace smirkly::chat::domain {
class UserId final {
 public:
  explicit UserId(boost::uuids::uuid value) noexcept : value_(value) {}

  const boost::uuids::uuid& Value() const noexcept { return value_; }

  friend bool operator==(const UserId&, const UserId&) = default;

 private:
  boost::uuids::uuid value_;
};
}  // namespace smirkly::chat::domain