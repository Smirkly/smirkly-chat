#pragma once

#include <boost/uuid/uuid.hpp>

namespace smirkly::chat::domain {
class ConversationId final {
 public:
  explicit ConversationId(boost::uuids::uuid value) noexcept : value_(value) {}

  const boost::uuids::uuid& Value() const noexcept { return value_; }

  friend bool operator==(const ConversationId&,
                         const ConversationId&) = default;

 private:
  boost::uuids::uuid value_;
};
}  // namespace smirkly::chat::domain