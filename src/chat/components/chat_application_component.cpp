#include <chat/components/chat_application_component.hpp>

#include <memory>
#include <string>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <chat/application/usecases/send_message_use_case.hpp>
#include <chat/infra/postgres/postgres_conversation_member_repository.hpp>
#include <chat/infra/postgres/postgres_message_repository.hpp>
#include <chat/infra/postgres/transactions/pg_transaction_manager.hpp>

namespace smirkly::chat::components {

struct ChatApplicationComponent::Impl final {
  infra::postgres::PgTransactionManager transaction_manager;
  infra::postgres::PostgresConversationMemberRepository member_repository;
  infra::postgres::PostgresMessageRepository message_repository;
  application::usecases::SendMessageUseCase send_message;

  Impl(const userver::components::ComponentConfig& config,
       const userver::components::ComponentContext& context)
      : transaction_manager(
            context
                .FindComponent<userver::components::Postgres>(
                    config["postgres-component"].As<std::string>(
                        "postgres-chat"))
                .GetCluster()),
        send_message(transaction_manager, member_repository,
                     message_repository) {}
};

ChatApplicationComponent::ChatApplicationComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      impl_(std::make_unique<Impl>(config, context)) {}

ChatApplicationComponent::~ChatApplicationComponent() = default;

application::usecases::SendMessageUseCase&
ChatApplicationComponent::GetSendMessageUseCase() noexcept {
  return impl_->send_message;
}

userver::yaml_config::Schema ChatApplicationComponent::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: Composes chat application use cases with PostgreSQL adapters
additionalProperties: false
properties:
    postgres-component:
        type: string
        description: PostgreSQL component used by chat application services
        default: postgres-chat
)");
}

}  // namespace smirkly::chat::components
