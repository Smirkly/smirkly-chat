#pragma once

#include <stdexcept>
#include <string>

namespace smirkly::chat::platform::auth {

struct AccessTokenClaims final {
  std::string user_id;
  std::string session_id;
};

class InvalidAccessToken final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class JwksUnavailable final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

}  // namespace smirkly::chat::platform::auth
