#include <chat/api/v0/websocket/chat_websocket_handler.hpp>
#include <chat/platform/auth/bearer_token.hpp>
#include <exception>
#include <string>
#include <userver/components/component_context.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>
#include <utility>

namespace smirkly::chat::api::v0 {
namespace {

std::string ConnectedMessage(const platform::auth::AccessTokenClaims& claims) {
  userver::formats::json::ValueBuilder response;
  response["type"] = "system.connected";
  response["payload"]["user_id"] = claims.user_id;
  response["payload"]["session_id"] = claims.session_id;
  return userver::formats::json::ToString(response.ExtractValue());
}

std::string EventMessage(std::string type) {
  userver::formats::json::ValueBuilder response;
  response["type"] = std::move(type);
  return userver::formats::json::ToString(response.ExtractValue());
}

std::string ErrorMessage(std::string code, std::string message) {
  userver::formats::json::ValueBuilder response;
  response["type"] = "error";
  response["payload"]["code"] = std::move(code);
  response["payload"]["message"] = std::move(message);
  return userver::formats::json::ToString(response.ExtractValue());
}

}  // namespace

ChatWebsocketHandler::ChatWebsocketHandler(const userver::components::ComponentConfig& config,
                                           const userver::components::ComponentContext& context)
    : WebsocketHandlerBase(config, context),
      verifier_(context.FindComponent<platform::auth::JwksVerifierComponent>()) {}

bool ChatWebsocketHandler::HandleHandshake(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
  auto& response = request.GetHttpResponse();
  const auto token =
      platform::auth::ExtractBearerToken(request.GetHeader(userver::http::headers::kAuthorization));
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
    response.SetHeader(userver::http::headers::kWWWAuthenticate, "Bearer error=\"invalid_token\"");
    return false;
  }
}

void ChatWebsocketHandler::Handle(userver::websocket::WebSocketConnection& websocket,
                                  userver::server::request::RequestContext& context) const {
  const auto& claims = context.GetUserData<platform::auth::AccessTokenClaims>();
  websocket.SendText(ConnectedMessage(claims));

  userver::websocket::Message message;
  while (!userver::engine::current_task::ShouldCancel()) {
    websocket.Recv(message);
    if (message.close_status) break;

    if (!message.is_text) {
      websocket.SendText(ErrorMessage("chat.unsupported_frame", "only text frames are supported"));
      continue;
    }

    try {
      const auto event = userver::formats::json::FromString(message.data);
      const auto type = event["type"].As<std::string>();
      if (type == "ping") {
        websocket.SendText(EventMessage("pong"));
      } else {
        websocket.SendText(ErrorMessage(
            "chat.not_implemented", "business WebSocket events are intentionally not implemented"));
      }
    } catch (const std::exception&) {
      websocket.SendText(ErrorMessage("chat.invalid_event", "invalid event envelope"));
    }
  }

  if (message.close_status) websocket.Close(*message.close_status);
}

}  // namespace smirkly::chat::api::v0
