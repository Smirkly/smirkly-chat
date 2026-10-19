#include <chat/api/v0/websocket/websocket_dispatcher.hpp>

#include <exception>
#include <string>
#include <string_view>
#include <utility>

#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>

#include <chat/api/v0/websocket/protocol/error.hpp>
#include <chat/api/v0/websocket/protocol/event_codec.hpp>
#include <chat/api/v0/websocket/protocol/message_events.hpp>
#include <chat/application/contracts/send_message.hpp>
#include <chat/application/errors/send_message.hpp>

namespace smirkly::chat::api::v0::websocket {
namespace {

std::string Error(std::string code, std::string message,
                  std::string_view request_id) {
  return protocol::EncodeError(
      {.code = std::move(code), .message = std::move(message)}, request_id);
}

}  // namespace

WebsocketDispatcher::WebsocketDispatcher(
    application::usecases::SendMessageUseCase& send_message)
    : send_message_(send_message) {}

std::string WebsocketDispatcher::Dispatch(
    const protocol::ClientEnvelope& envelope,
    const domain::UserId& authenticated_user) const {
  if (envelope.type == "ping") {
    return protocol::EncodeServerEnvelope({
        .type = "pong",
        .request_id = envelope.request_id,
        .payload = userver::formats::json::MakeObject(),
    });
  }

  if (envelope.type != "message.send") {
    return Error("chat.unsupported_event", "event type is not supported",
                 envelope.request_id);
  }

  try {
    auto payload = protocol::DecodeSendMessagePayload(envelope.payload);
    const application::contracts::SendMessageCommand command{
        .author_id = authenticated_user,
        .conversation_id = payload.conversation_id,
        .client_message_id = payload.client_message_id,
        .text = std::move(payload.text),
    };
    auto result = send_message_.Execute(command);

    return protocol::EncodeServerEnvelope({
        .type = "message.created",
        .request_id = envelope.request_id,
        .payload = protocol::EncodeMessageCreatedPayload(result.message),
    });
  } catch (const protocol::InvalidEventPayload&) {
    return Error("chat.invalid_payload", "invalid message.send payload",
                 envelope.request_id);
  } catch (const application::errors::InvalidMessageText&) {
    return Error("chat.invalid_payload", "message text is invalid",
                 envelope.request_id);
  } catch (const application::errors::MessageTooLong&) {
    return Error("chat.message_too_long",
                 "message text exceeds the maximum size", envelope.request_id);
  } catch (const application::errors::ConversationAccessDenied&) {
    return Error("chat.conversation_access_denied",
                 "conversation does not exist or access is denied",
                 envelope.request_id);
  } catch (const application::errors::IdempotencyConflict&) {
    return Error("chat.idempotency_conflict",
                 "client_message_id was already used for another message",
                 envelope.request_id);
  } catch (const std::exception& error) {
    LOG_ERROR() << "message.send failed: " << error.what();
    return Error("chat.internal_error", "internal server error",
                 envelope.request_id);
  }
}

}  // namespace smirkly::chat::api::v0::websocket
