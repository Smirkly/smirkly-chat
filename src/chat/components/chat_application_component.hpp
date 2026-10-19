#pragma once

#include <memory>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/yaml_config/schema.hpp>

namespace smirkly::chat::application::usecases {
class SendMessageUseCase;
}  // namespace smirkly::chat::application::usecases

namespace smirkly::chat::components {

class ChatApplicationComponent final
    : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "chat-application";

  ChatApplicationComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& context);
  ~ChatApplicationComponent() override;

  ChatApplicationComponent(const ChatApplicationComponent&) = delete;
  ChatApplicationComponent& operator=(const ChatApplicationComponent&) = delete;

  [[nodiscard]] application::usecases::SendMessageUseCase&
  GetSendMessageUseCase() noexcept;

  static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace smirkly::chat::components
