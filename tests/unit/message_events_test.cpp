#include <chat/api/v0/websocket/protocol/message_events.hpp>

#include <chrono>
#include <string>

#include <boost/uuid/string_generator.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/utest.hpp>

#include <chat/api/v0/websocket/protocol/error.hpp>

namespace smirkly::chat::api::v0::websocket::protocol {
namespace {

userver::formats::json::Value ValidPayload() {
  return userver::formats::json::FromString(R"json(
    {
      "conversation_id": "8d877cac-2aef-4bb4-b48c-766b7f040021",
      "client_message_id": "af762c44-1d77-4f02-ad90-4702101de560",
      "text": "Hello, Smirkly!"
    }
  )json");
}

UTEST(MessageEvents, DecodesSendMessagePayload) {
  const auto payload = DecodeSendMessagePayload(ValidPayload());

  EXPECT_EQ(
      payload.conversation_id.Value(),
      boost::uuids::string_generator{}("8d877cac-2aef-4bb4-b48c-766b7f040021"));
  EXPECT_EQ(
      payload.client_message_id.Value(),
      boost::uuids::string_generator{}("af762c44-1d77-4f02-ad90-4702101de560"));
  EXPECT_EQ(payload.text, "Hello, Smirkly!");
}

UTEST(MessageEvents, RejectsNonObjectSendMessagePayload) {
  const auto payload = userver::formats::json::FromString("[]");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsMissingConversationId) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "client_message_id": "af762c44-1d77-4f02-ad90-4702101de560",
      "text": "Hello"
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsNonStringConversationId) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "conversation_id": 42,
      "client_message_id": "af762c44-1d77-4f02-ad90-4702101de560",
      "text": "Hello"
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsInvalidConversationId) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "conversation_id": "not-a-uuid",
      "client_message_id": "af762c44-1d77-4f02-ad90-4702101de560",
      "text": "Hello"
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsNonCanonicalConversationId) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "conversation_id": "8D877CAC-2Aef-4bb4-b48c-766b7f040021",
      "client_message_id": "af762c44-1d77-4f02-ad90-4702101de560",
      "text": "Hello"
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsMissingClientMessageId) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "conversation_id": "8d877cac-2aef-4bb4-b48c-766b7f040021",
      "text": "Hello"
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsNonStringClientMessageId) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "conversation_id": "8d877cac-2aef-4bb4-b48c-766b7f040021",
      "client_message_id": {},
      "text": "Hello"
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsInvalidClientMessageId) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "conversation_id": "8d877cac-2aef-4bb4-b48c-766b7f040021",
      "client_message_id": "not-a-uuid",
      "text": "Hello"
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsNonCanonicalClientMessageId) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "conversation_id": "8d877cac-2aef-4bb4-b48c-766b7f040021",
      "client_message_id": "AF762C44-1d77-4f02-ad90-4702101de560",
      "text": "Hello"
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsMissingText) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "conversation_id": "8d877cac-2aef-4bb4-b48c-766b7f040021",
      "client_message_id": "af762c44-1d77-4f02-ad90-4702101de560"
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, RejectsNonStringText) {
  const auto payload = userver::formats::json::FromString(R"json(
    {
      "conversation_id": "8d877cac-2aef-4bb4-b48c-766b7f040021",
      "client_message_id": "af762c44-1d77-4f02-ad90-4702101de560",
      "text": ["Hello"]
    }
  )json");

  EXPECT_THROW(DecodeSendMessagePayload(payload), InvalidEventPayload);
}

UTEST(MessageEvents, PreservesEmptyTextForApplicationValidation) {
  auto payload = userver::formats::json::ValueBuilder{ValidPayload()};
  payload["text"] = "";

  const auto decoded = DecodeSendMessagePayload(payload.ExtractValue());

  EXPECT_TRUE(decoded.text.empty());
}

UTEST(MessageEvents, PreservesLongTextForApplicationValidation) {
  auto payload = userver::formats::json::ValueBuilder{ValidPayload()};
  payload["text"] = std::string(4097, 'a');

  const auto decoded = DecodeSendMessagePayload(payload.ExtractValue());

  EXPECT_EQ(decoded.text.size(), 4097);
}

UTEST(MessageEvents, EncodesMessageCreatedPayload) {
  const domain::Message message{
      domain::MessageId{boost::uuids::string_generator{}(
          "85e97f15-dd0f-4d26-a57b-0ea8164d2139")},
      domain::ClientMessageId{boost::uuids::string_generator{}(
          "af762c44-1d77-4f02-ad90-4702101de560")},
      domain::ConversationId{boost::uuids::string_generator{}(
          "8d877cac-2aef-4bb4-b48c-766b7f040021")},
      domain::UserId{boost::uuids::string_generator{}(
          "7da5fd82-8da1-455f-8c66-0680c821f062")},
      "Hello, Smirkly!",
      std::chrono::system_clock::time_point{std::chrono::microseconds{123456}},
  };

  const auto payload = EncodeMessageCreatedPayload(message);

  EXPECT_EQ(payload["message_id"].As<std::string>(),
            "85e97f15-dd0f-4d26-a57b-0ea8164d2139");
  EXPECT_EQ(payload["client_message_id"].As<std::string>(),
            "af762c44-1d77-4f02-ad90-4702101de560");
  EXPECT_EQ(payload["conversation_id"].As<std::string>(),
            "8d877cac-2aef-4bb4-b48c-766b7f040021");
  EXPECT_EQ(payload["author_id"].As<std::string>(),
            "7da5fd82-8da1-455f-8c66-0680c821f062");
  EXPECT_EQ(payload["text"].As<std::string>(), "Hello, Smirkly!");
  EXPECT_EQ(payload["created_at"].As<std::string>(),
            "1970-01-01T00:00:00.123456+00:00");
}

}  // namespace
}  // namespace smirkly::chat::api::v0::websocket::protocol
