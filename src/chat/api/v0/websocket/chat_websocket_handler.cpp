#include <chat/api/v0/websocket/chat_websocket_handler.hpp>

#include <chrono>
#include <string>
#include <utility>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/boost_uuid4.hpp>

#include <chat/api/v0/websocket/protocol/event_codec.hpp>
#include <chat/components/chat_application_component.hpp>
#include <chat/domain/types/user_id.hpp>
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

struct ConnectionRegistration final {
  application::delivery::ConnectionRegistry& registry;
  std::string user_id;
  std::string connection_id;

  ~ConnectionRegistration() {
    registry.Unregister(user_id, connection_id);
  }
};

void DrainMailbox(userver::websocket::WebSocketConnection& websocket,
                  const application::delivery::ConnectionRegistry::MailboxPtr&
                      mailbox) {
  std::string event;
  while (mailbox->TryPop(event)) {
    websocket.SendText(event);
  }
}

}  // namespace

ChatWebsocketHandler::ChatWebsocketHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : WebsocketHandlerBase(config, context),
      verifier_(context.FindComponent<platform::auth::JwksVerifierComponent>()),
      dispatcher_(context.FindComponent<components::ChatApplicationComponent>()
                      .GetSendMessageUseCase(),
                  context.FindComponent<components::ChatApplicationComponent>()
                      .GetConnectionRegistry()),
      connection_registry_(
          context.FindComponent<components::ChatApplicationComponent>()
              .GetConnectionRegistry()) {}

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
  const auto remaining = claims.expires_at - std::chrono::system_clock::now();
  const auto token_deadline =
      userver::engine::Deadline::FromDuration(remaining);

  if (token_deadline.IsReached()) {
    websocket.Close(userver::websocket::CloseStatus::kPolicyViolation);
    return;
  }

  const auto connection_id =
      boost::uuids::to_string(boost::uuids::random_generator{}());
  const auto mailbox =
      connection_registry_.Register(claims.user_id, connection_id);
  [[maybe_unused]] const ConnectionRegistration registration{
      connection_registry_, claims.user_id, connection_id};

  websocket.SendText(ConnectedMessage(claims));
  const domain::UserId authenticated_user{
      userver::utils::BoostUuidFromString(claims.user_id)};

  while (!userver::engine::current_task::ShouldCancel()) {
    if (token_deadline.IsReached() ||
        std::chrono::system_clock::now() >= claims.expires_at) {
      websocket.Close(userver::websocket::CloseStatus::kPolicyViolation);
      return;
    }

    auto receive_task =
        userver::utils::Async("websocket-receive", [&websocket] {
          userver::websocket::Message incoming;
          websocket.Recv(incoming);
          return incoming;
        });

    userver::websocket::Message message;
    bool received = false;
    while (!received && !userver::engine::current_task::ShouldCancel()) {
      const auto poll_deadline = userver::engine::Deadline::FromDuration(
          std::chrono::milliseconds{100});
      const auto status = receive_task.WaitNothrowUntil(poll_deadline);
      switch (status) {
        case userver::engine::FutureStatus::kCancelled:
          receive_task.SyncCancel();
          return;
        case userver::engine::FutureStatus::kTimeout:
          if (token_deadline.IsReached() ||
              std::chrono::system_clock::now() >= claims.expires_at) {
            receive_task.SyncCancel();
            websocket.Close(userver::websocket::CloseStatus::kPolicyViolation);
            return;
          }
          DrainMailbox(websocket, mailbox);
          break;
        case userver::engine::FutureStatus::kReady:
          message = receive_task.Get();
          received = true;
          break;
      }
    }

    if (userver::engine::current_task::ShouldCancel()) {
      receive_task.SyncCancel();
      return;
    }
    if (!received) continue;

    if (message.close_status) {
      websocket.Close(*message.close_status);
      return;
    }

    if (!message.is_text) {
      websocket.SendText(websocket::protocol::EncodeError(
          {.code = "chat.unsupported_frame",
           .message = "only text frames are supported"},
          std::nullopt));
      DrainMailbox(websocket, mailbox);
      continue;
    }

    try {
      const auto client_envelope =
          websocket::protocol::DecodeClientEnvelope(message.data);
      websocket.SendText(
          dispatcher_.Dispatch(client_envelope, authenticated_user,
                               connection_id));
    } catch (const websocket::protocol::InvalidEnvelope&) {
      websocket.SendText(websocket::protocol::EncodeError(
          {.code = "chat.invalid_event", .message = "invalid event envelope"},
          std::nullopt));
    }
    DrainMailbox(websocket, mailbox);
  }

  if (userver::engine::current_task::ShouldCancel()) {
    return;
  }
}

}  // namespace smirkly::chat::api::v0
