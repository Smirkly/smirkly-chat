#include <chat/api/v0/websocket/protocol/message_events.hpp>

#include <exception>
#include <string>
#include <string_view>

#include <boost/uuid/uuid.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/boost_uuid4.hpp>
#include <userver/utils/datetime_light.hpp>

#include <chat/api/v0/websocket/protocol/error.hpp>

namespace smirkly::chat::api::v0::websocket::protocol {
namespace {

std::string ReadString(const userver::formats::json::Value& payload,
                       std::string_view field) {
  if (!payload.HasMember(field) || !payload[field].IsString()) {
    throw InvalidEventPayload(std::string{field} + " must be a string");
  }
  return payload[field].As<std::string>();
}

boost::uuids::uuid ReadCanonicalUuid(
    const userver::formats::json::Value& payload, std::string_view field) {
  const auto value = ReadString(payload, field);
  try {
    const auto uuid = userver::utils::BoostUuidFromString(value);
    if (userver::utils::ToString(uuid) != value) {
      throw InvalidEventPayload(std::string{field} +
                                " must be a canonical UUID");
    }
    return uuid;
  } catch (const InvalidEventPayload&) {
    throw;
  } catch (const std::exception&) {
    throw InvalidEventPayload(std::string{field} + " must be a valid UUID");
  }
}

}  // namespace

SendMessagePayload DecodeSendMessagePayload(
    const userver::formats::json::Value& payload) {
  if (!payload.IsObject()) {
    throw InvalidEventPayload("message.send payload must be an object");
  }

  return {
      .conversation_id =
          domain::ConversationId{ReadCanonicalUuid(payload, "conversation_id")},
      .client_message_id = domain::ClientMessageId{ReadCanonicalUuid(
          payload, "client_message_id")},
      .text = ReadString(payload, "text"),
  };
}

userver::formats::json::Value EncodeMessageCreatedPayload(
    const domain::Message& message) {
  userver::formats::json::ValueBuilder payload;
  payload["message_id"] = userver::utils::ToString(message.GetId().Value());
  payload["client_message_id"] =
      userver::utils::ToString(message.GetClientMessageId().Value());
  payload["conversation_id"] =
      userver::utils::ToString(message.GetConversationId().Value());
  payload["author_id"] =
      userver::utils::ToString(message.GetAuthorId().Value());
  payload["text"] = message.GetText();
  payload["created_at"] = userver::utils::datetime::UtcTimestring(
      message.GetCreatedAt(), userver::utils::datetime::kRfc3339Format);
  return payload.ExtractValue();
}

}  // namespace smirkly::chat::api::v0::websocket::protocol
