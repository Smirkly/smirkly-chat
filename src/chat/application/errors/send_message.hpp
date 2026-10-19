#pragma once

#include <stdexcept>

namespace smirkly::chat::application::errors {
class InvalidMessageText final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class MessageTooLong final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class ConversationAccessDenied final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class IdempotencyConflict final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};
}  // namespace smirkly::chat::application::errors
