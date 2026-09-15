#pragma once

#include <string>

#include <userver/formats/json/value.hpp>

#include <chat/domain/message.hpp>
#include <chat/domain/types/client_message_id.hpp>
#include <chat/domain/types/conversation_id.hpp>

namespace smirkly::chat::api::v0::websocket::protocol {

struct SendMessagePayload final {
  domain::ConversationId conversation_id;
  domain::ClientMessageId client_message_id;
  std::string text;
};

[[nodiscard]] SendMessagePayload DecodeSendMessagePayload(
    const userver::formats::json::Value& payload);

[[nodiscard]] userver::formats::json::Value EncodeMessageCreatedPayload(
    const domain::Message& message);

}  // namespace smirkly::chat::api::v0::websocket::protocol
