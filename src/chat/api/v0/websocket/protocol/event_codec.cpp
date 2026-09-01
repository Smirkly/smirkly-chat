#include <chat/api/v0/websocket/protocol/event_codec.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace smirkly::chat::api::v0::websocket::protocol {

ClientEnvelope DecodeClientEnvelope(std::string_view data) try {
  const auto root = userver::formats::json::FromString(data);
  if (!root.IsObject()) {
    throw InvalidEnvelope("envelope must be an object");
  }

  if (!root.HasMember("type") || !root["type"].IsString()) {
    throw InvalidEnvelope("type must be a string");
  }

  if (!root.HasMember("request_id") || !root["request_id"].IsString()) {
    throw InvalidEnvelope("request_id must be a string");
  }

  if (!root.HasMember("payload") || !root["payload"].IsObject()) {
    throw InvalidEnvelope("payload must be an object");
  }

  const auto type = root["type"].As<std::string>();
  const auto request_id = root["request_id"].As<std::string>();

  if (type.empty() || type.size() > 64) {
    throw InvalidEnvelope("type length must be between 1 and 64");
  }

  if (request_id.empty() || request_id.size() > 64) {
    throw InvalidEnvelope("request_id length must be between 1 and 64");
  }

  return {
      .type = type,
      .request_id = request_id,
      .payload = root["payload"],
  };
} catch (const userver::formats::json::Exception&) {
  throw InvalidEnvelope("malformed JSON envelope");
}

std::string EncodeServerEnvelope(const ServerEnvelope& envelope) {
  userver::formats::json::ValueBuilder builder;
  builder["type"] = envelope.type;

  if (envelope.request_id) {
    builder["request_id"] = *envelope.request_id;
  }

  builder["payload"] = envelope.payload;

  return userver::formats::json::ToString(builder.ExtractValue());
}

std::string EncodeError(const ProtocolError& error,
                        std::optional<std::string_view> request_id) {
  userver::formats::json::ValueBuilder payload;
  payload["code"] = error.code;
  payload["message"] = error.message;

  std::optional<std::string> owned_request_id;
  if (request_id) {
    owned_request_id = *request_id;
  }

  return EncodeServerEnvelope(ServerEnvelope{
      .type = "error",
      .request_id = owned_request_id,
      .payload = payload.ExtractValue(),
  });
}
}  // namespace smirkly::chat::api::v0::websocket::protocol
