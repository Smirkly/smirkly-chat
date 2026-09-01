#include <chat/api/v0/websocket/chat_websocket_handler.hpp>

#include <string>

#include <userver/components/component_context.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>

#include <chat/api/v0/websocket/protocol/event_codec.hpp>
#include <chat/platform/auth/bearer_token.hpp>

namespace smirkly::chat::api::v0 {
namespace {

std::string ConnectedMessage(const platform::auth::AccessTokenClaims& claims) {
  userver::formats::json::ValueBuilder payload;
  payload["user_id"] = claims.user_id;
  payload["session_id"] = claims.session_id;

  return websocket::protocol::EncodeServerEnvelope({
      .type = "system.connected",
      .request_id = std::nullopt,
      .payload = payload.ExtractValue(),
  });
}
}  // namespace

ChatWebsocketHandler::ChatWebsocketHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : WebsocketHandlerBase(config, context),
      verifier_(
          context.FindComponent<platform::auth::JwksVerifierComponent>()) {}

bool ChatWebsocketHandler::HandleHandshake(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
  auto& response = request.GetHttpResponse();
  const auto token = platform::auth::ExtractBearerToken(
      request.GetHeader(userver::http::headers::kAuthorization));
  if (!token) {
    response.SetStatus(userver::server::http::HttpStatus::kUnauthorized);
    response.SetHeader(userver::http::headers::kWWWAuthenticate, "Bearer");
    return false;
  }

  try {
    context.SetUserData(verifier_.Verify(*token));
    return true;
  } catch (const platform::auth::JwksUnavailable& error) {
    LOG_WARNING() << "WebSocket authentication unavailable: " << error.what();
    response.SetStatus(userver::server::http::HttpStatus::kServiceUnavailable);
    return false;
  } catch (const platform::auth::InvalidAccessToken&) {
    response.SetStatus(userver::server::http::HttpStatus::kUnauthorized);
    response.SetHeader(userver::http::headers::kWWWAuthenticate,
                       "Bearer error=\"invalid_token\"");
    return false;
  }
}

void ChatWebsocketHandler::Handle(
    userver::websocket::WebSocketConnection& websocket,
    userver::server::request::RequestContext& context) const {
  const auto& claims = context.GetUserData<platform::auth::AccessTokenClaims>();
  websocket.SendText(ConnectedMessage(claims));

  userver::websocket::Message message;
  while (!userver::engine::current_task::ShouldCancel()) {
    websocket.Recv(message);
    if (message.close_status) break;

    if (!message.is_text) {
      websocket.SendText(websocket::protocol::EncodeError(
          {.code = "chat.unsupported_frame",
           .message = "only text frames are supported"},
          std::nullopt));
      continue;
    }

    try {
      const auto client_envelope =
          websocket::protocol::DecodeClientEnvelope(message.data);

      if (client_envelope.type == "ping") {
        websocket.SendText(websocket::protocol::EncodeServerEnvelope(
            {.type = "pong",
             .request_id = client_envelope.request_id,
             .payload = userver::formats::json::MakeObject()}));
      } else {
        websocket.SendText(websocket::protocol::EncodeError(
            {.code = "chat.unsupported_event",
             .message = "event type is not supported"},
            client_envelope.request_id));
      }
    } catch (const websocket::protocol::InvalidEnvelope&) {
      websocket.SendText(websocket::protocol::EncodeError(
          {.code = "chat.invalid_event", .message = "invalid event envelope"},
          std::nullopt));
    }
  }

  if (message.close_status) websocket.Close(*message.close_status);
}

}  // namespace smirkly::chat::api::v0
