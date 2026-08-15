#pragma once

#include <memory>
#include <string_view>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/redis/client_fwd.hpp>
#include <userver/yaml_config/schema.hpp>

namespace smirkly::chat::api::health {

class ReadinessHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-health-ready";

  ReadinessHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

  static userver::yaml_config::Schema GetStaticConfigSchema();

  Value HandleRequestJsonThrow(const HttpRequest& request, const Value& body,
                               RequestContext& context) const override;

 private:
  userver::storages::postgres::ClusterPtr postgres_;
  userver::storages::redis::ClientPtr redis_;
};

}  // namespace smirkly::chat::api::health
