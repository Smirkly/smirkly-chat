#include <chat/api/health/readiness_handler.hpp>
#include <exception>
#include <string>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/query.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace smirkly::chat::api::health {

ReadinessHandler::ReadinessHandler(const userver::components::ComponentConfig& config,
                                   const userver::components::ComponentContext& context)
    : HttpHandlerJsonBase(config, context),
      postgres_(context
                    .FindComponent<userver::components::Postgres>(
                        config["postgres-component"].As<std::string>())
                    .GetCluster()),
      redis_(context
                 .FindComponent<userver::components::Redis>(
                     config["redis-component"].As<std::string>())
                 .GetClient(config["redis-client-name"].As<std::string>())) {}

ReadinessHandler::Value ReadinessHandler::HandleRequestJsonThrow(const HttpRequest& request,
                                                                 const Value&,
                                                                 RequestContext&) const {
  bool postgres_ok = false;
  bool redis_ok = false;

  try {
    const auto result = postgres_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "SELECT 1", userver::storages::postgres::Query::Name{"health.postgres.ping"}});
    postgres_ok = result.AsSingleRow<int>() == 1;
  } catch (const std::exception& error) {
    LOG_WARNING() << "PostgreSQL readiness check failed: " << error.what();
  }

  try {
    redis_->Ping(0, {}).Get();
    redis_ok = true;
  } catch (const std::exception& error) {
    LOG_WARNING() << "Redis readiness check failed: " << error.what();
  }

  const bool ready = postgres_ok && redis_ok;
  request.GetHttpResponse().SetStatus(ready
                                          ? userver::server::http::HttpStatus::kOk
                                          : userver::server::http::HttpStatus::kServiceUnavailable);

  userver::formats::json::ValueBuilder response;
  response["status"] = ready ? "ready" : "not_ready";
  response["checks"]["postgres"] = postgres_ok;
  response["checks"]["redis"] = redis_ok;
  return response.ExtractValue();
}

userver::yaml_config::Schema ReadinessHandler::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<HttpHandlerJsonBase>(R"(
type: object
description: Checks mandatory chat infrastructure
additionalProperties: false
properties:
    postgres-component:
        type: string
        description: PostgreSQL component used by the readiness probe
        default: postgres-chat
    redis-component:
        type: string
        description: Redis component used by the readiness probe
        default: redis-chat
    redis-client-name:
        type: string
        description: Redis client name inside the selected Redis component
        default: chat-redis
)");
}

}  // namespace smirkly::chat::api::health
