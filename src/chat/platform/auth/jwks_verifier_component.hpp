#pragma once

#include <chat/platform/auth/access_token.hpp>
#include <memory>
#include <string_view>
#include <userver/components/component_base.hpp>
#include <userver/yaml_config/schema.hpp>

namespace smirkly::chat::platform::auth {

class JwksVerifierComponent final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "chat-jwks-verifier";

  JwksVerifierComponent(const userver::components::ComponentConfig& config,
                        const userver::components::ComponentContext& context);
  ~JwksVerifierComponent() override;

  JwksVerifierComponent(const JwksVerifierComponent&) = delete;
  JwksVerifierComponent& operator=(const JwksVerifierComponent&) = delete;

  [[nodiscard]] AccessTokenClaims Verify(std::string_view token) const;

  static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace smirkly::chat::platform::auth
