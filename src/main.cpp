#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/clients/http/middlewares/pipeline_component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/kafka/producer_component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#if SMIRKLY_CHAT_ENABLE_TESTSUITE
#include <userver/server/handlers/tests_control.hpp>
#endif
#include <chat/api/health/liveness_handler.hpp>
#include <chat/api/health/readiness_handler.hpp>
#include <chat/api/v0/websocket/chat_websocket_handler.hpp>
#include <chat/platform/auth/jwks_verifier_component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#if SMIRKLY_CHAT_ENABLE_TESTSUITE
#include <userver/testsuite/testsuite_support.hpp>
#endif
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
  auto components = userver::components::MinimalServerComponentList()
                        .Append<userver::components::Postgres>("postgres-chat")
                        .Append<userver::components::Secdist>()
                        .Append<userver::components::DefaultSecdistProvider>()
                        .Append<userver::components::Redis>("redis-chat")
                        .Append<userver::kafka::ProducerComponent>("kafka-producer-chat")
                        .Append<userver::clients::dns::Component>()
                        .Append<userver::components::HttpClientCore>()
                        .Append<userver::clients::http::MiddlewarePipelineComponent>()
                        .Append<userver::components::HttpClient>()
                        .Append<userver::server::handlers::Ping>()
                        .Append<userver::server::handlers::ServerMonitor>()
#if SMIRKLY_CHAT_ENABLE_TESTSUITE
                        .Append<userver::components::TestsuiteSupport>()
                        .Append<userver::server::handlers::TestsControl>()
#endif
                        .Append<smirkly::chat::platform::auth::JwksVerifierComponent>()
                        .Append<smirkly::chat::api::health::LivenessHandler>()
                        .Append<smirkly::chat::api::health::ReadinessHandler>()
                        .Append<smirkly::chat::api::v0::ChatWebsocketHandler>();

  return userver::utils::DaemonMain(argc, argv, components);
}
