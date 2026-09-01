#pragma once

#include <optional>
#include <string_view>

namespace smirkly::chat::platform::auth {

inline std::optional<std::string_view> ExtractBearerToken(
    std::string_view authorization) {
  constexpr std::string_view kPrefix = "Bearer ";
  if (!authorization.starts_with(kPrefix)) {
    return std::nullopt;
  }

  const auto token = authorization.substr(kPrefix.size());
  if (token.empty() ||
      token.find_first_of(" \t\r\n") != std::string_view::npos) {
    return std::nullopt;
  }
  return token;
}

}  // namespace smirkly::chat::platform::auth
