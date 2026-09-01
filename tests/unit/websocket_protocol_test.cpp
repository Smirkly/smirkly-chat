#include <chat/api/v0/websocket/protocol/event_codec.hpp>

#include <optional>
#include <string>
#include <string_view>

#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/utest.hpp>

namespace smirkly::chat::api::v0::websocket::protocol {
namespace {
UTEST(WebsocketProtocol, DecodesValidEnvelope) {
  const auto envelope = DecodeClientEnvelope(R"json(
      {
        "type": "ping",
        "request_id": "request-123",
        "payload": {
          "value": 42
        }
      }
    )json");

  EXPECT_EQ(envelope.type, "ping");
  EXPECT_EQ(envelope.request_id, "request-123");
  ASSERT_TRUE(envelope.payload.IsObject());
  EXPECT_EQ(envelope.payload["value"].As<int>(), 42);
}

UTEST(WebsocketProtocol, RejectsMalformedJson) {
  EXPECT_THROW(DecodeClientEnvelope(R"json(
          {
            "type": "ping",
            "request_id" "request-123",
            "payload": {}
          }
        )json"),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, RejectsNonObjectPayload) {
  EXPECT_THROW(DecodeClientEnvelope(R"json(
      {
        "type": "ping",
        "request_id": "request-123",
        "payload": []
      }
    )json"),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, RejectsMissingType) {
  EXPECT_THROW(DecodeClientEnvelope(R"json(
      {
        "request_id": "request-123",
        "payload": {
          "value": 42
        }
      }
    )json"),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, RejectsNonStringType) {
  EXPECT_THROW(DecodeClientEnvelope(R"json(
      {
        "type": 67,
        "request_id": "request-123",
        "payload": {
          "value": 42
        }
      }
    )json"),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, RejectsEmptyType) {
  EXPECT_THROW(DecodeClientEnvelope(R"json(
      {
        "type": "",
        "request_id": "request-123",
        "payload": {
          "value": 42
        }
      }
    )json"),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, RejectsTypeAboveMaximumLength) {
  userver::formats::json::ValueBuilder json;
  json["type"] = std::string(65, 'a');
  json["request_id"] = "request-123";
  json["payload"]["value"] = 42;

  EXPECT_THROW(DecodeClientEnvelope(
                   userver::formats::json::ToString(json.ExtractValue())),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, AcceptsTypeAtMaximumLength) {
  const std::string type(64, 'a');
  userver::formats::json::ValueBuilder json;
  json["type"] = type;
  json["request_id"] = "request-123";
  json["payload"] = userver::formats::json::MakeObject();

  const auto envelope = DecodeClientEnvelope(
      userver::formats::json::ToString(json.ExtractValue()));

  EXPECT_EQ(envelope.type, type);
}

UTEST(WebsocketProtocol, RejectsMissingRequestId) {
  EXPECT_THROW(DecodeClientEnvelope(R"json(
      {
        "type": "ping",
        "payload": {
          "value": 42
        }
      }
    )json"),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, RejectsNonStringRequestId) {
  EXPECT_THROW(DecodeClientEnvelope(R"json(
      {
        "type": "ping",
        "request_id": 67,
        "payload": {
          "value": 42
        }
      }
    )json"),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, RejectsEmptyRequestId) {
  EXPECT_THROW(DecodeClientEnvelope(R"json(
      {
        "type": "ping",
        "request_id": "",
        "payload": {
          "value": 42
        }
      }
    )json"),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, RejectsRequestIdAboveMaximumLength) {
  userver::formats::json::ValueBuilder json;
  json["type"] = "ping";
  json["request_id"] = std::string(65, 'a');
  json["payload"]["value"] = 42;

  EXPECT_THROW(DecodeClientEnvelope(
                   userver::formats::json::ToString(json.ExtractValue())),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, AcceptsRequestIdAtMaximumLength) {
  const std::string request_id(64, 'a');
  userver::formats::json::ValueBuilder json;
  json["type"] = "ping";
  json["request_id"] = request_id;
  json["payload"] = userver::formats::json::MakeObject();

  const auto envelope = DecodeClientEnvelope(
      userver::formats::json::ToString(json.ExtractValue()));

  EXPECT_EQ(envelope.request_id, request_id);
}

UTEST(WebsocketProtocol, RejectsMissingPayload) {
  EXPECT_THROW(DecodeClientEnvelope(R"json(
  {
    "type": "ping",
    "request_id": "request-123"
  }
  )json"),
               InvalidEnvelope);
}

UTEST(WebsocketProtocol, RejectsNonObjectRoot) {
  EXPECT_THROW(DecodeClientEnvelope(R"json([])json"), InvalidEnvelope);
}

UTEST(WebsocketProtocol, AcceptsUnknownEventType) {
  const auto envelope = DecodeClientEnvelope(R"json(
      {
        "type": "future.unknown",
        "request_id": "request-123",
        "payload": {
          "value": 42
        }
      }
    )json");

  EXPECT_EQ(envelope.type, "future.unknown");
  EXPECT_EQ(envelope.request_id, "request-123");
  ASSERT_TRUE(envelope.payload.IsObject());
  EXPECT_EQ(envelope.payload["value"].As<int>(), 42);
}

UTEST(WebsocketProtocol, EncodesServerEnvelopeWithRequestId) {
  const auto encoded = EncodeServerEnvelope({
      .type = "message.created",
      .request_id = std::string{"request-123"},
      .payload = userver::formats::json::MakeObject("value", 42),
  });

  const auto json = userver::formats::json::FromString(encoded);

  ASSERT_TRUE(json.IsObject());
  EXPECT_EQ(json["type"].As<std::string>(), "message.created");
  EXPECT_EQ(json["request_id"].As<std::string>(), "request-123");
  ASSERT_TRUE(json["payload"].IsObject());
  EXPECT_EQ(json["payload"]["value"].As<int>(), 42);
}

UTEST(WebsocketProtocol, EncodesServerEnvelopeWithoutRequestId) {
  const auto encoded = EncodeServerEnvelope({
      .type = "system.connected",
      .request_id = std::nullopt,
      .payload = userver::formats::json::MakeObject("value", 42),
  });

  const auto json = userver::formats::json::FromString(encoded);

  ASSERT_TRUE(json.IsObject());
  EXPECT_EQ(json["type"].As<std::string>(), "system.connected");
  EXPECT_FALSE(json.HasMember("request_id"));
  ASSERT_TRUE(json["payload"].IsObject());
  EXPECT_EQ(json["payload"]["value"].As<int>(), 42);
}

UTEST(WebsocketProtocol, EncodesErrorWithRequestId) {
  const auto encoded =
      EncodeError({.code = "chat.test_error", .message = "test error"},
                  std::string_view{"request-123"});

  const auto json = userver::formats::json::FromString(encoded);

  ASSERT_TRUE(json.IsObject());
  EXPECT_EQ(json["type"].As<std::string>(), "error");
  EXPECT_EQ(json["request_id"].As<std::string>(), "request-123");
  ASSERT_TRUE(json["payload"].IsObject());
  EXPECT_EQ(json["payload"]["code"].As<std::string>(), "chat.test_error");
  EXPECT_EQ(json["payload"]["message"].As<std::string>(), "test error");
}

UTEST(WebsocketProtocol, EncodesErrorWithoutRequestId) {
  const auto encoded = EncodeError(
      {.code = "chat.test_error", .message = "test error"}, std::nullopt);

  const auto json = userver::formats::json::FromString(encoded);

  ASSERT_TRUE(json.IsObject());
  EXPECT_EQ(json["type"].As<std::string>(), "error");
  EXPECT_FALSE(json.HasMember("request_id"));
  ASSERT_TRUE(json["payload"].IsObject());
  EXPECT_EQ(json["payload"]["code"].As<std::string>(), "chat.test_error");
  EXPECT_EQ(json["payload"]["message"].As<std::string>(), "test error");
}

}  // namespace
}  // namespace smirkly::chat::api::v0::websocket::protocol
