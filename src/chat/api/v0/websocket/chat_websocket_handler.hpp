#pragma once

#include <string_view>

#include <userver/server/handlers/websocket_handler.hpp>

#include <chat/api/v0/websocket/websocket_dispatcher.hpp>
#include <chat/application/delivery/connection_registry.hpp>
#include <chat/platform/auth/jwks_verifier_component.hpp>

namespace smirkly::chat::api::v0 {

class ChatWebsocketHandler final
    : public userver::server::handlers::WebsocketHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-chat-websocket";

  ChatWebsocketHandler(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

  bool HandleHandshake(
      userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

  void Handle(userver::websocket::WebSocketConnection& websocket,
              userver::server::request::RequestContext& context) const override;

 private:
  const platform::auth::JwksVerifierComponent& verifier_;
  websocket::WebsocketDispatcher dispatcher_;
  application::delivery::ConnectionRegistry& connection_registry_;
};

}  // namespace smirkly::chat::api::v0
