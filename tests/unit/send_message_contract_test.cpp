#include <chat/application/contracts/send_message.hpp>

#include <chrono>
#include <string>

#include <boost/uuid/string_generator.hpp>
#include <userver/utest/utest.hpp>

namespace smirkly::chat::application::contracts {
namespace {

domain::Message MakeMessage() {
  return {
      domain::MessageId{boost::uuids::string_generator{}(
          "85e97f15-dd0f-4d26-a57b-0ea8164d2139")},
      domain::ClientMessageId{boost::uuids::string_generator{}(
          "af762c44-1d77-4f02-ad90-4702101de560")},
      domain::ConversationId{boost::uuids::string_generator{}(
          "8d877cac-2aef-4bb4-b48c-766b7f040021")},
      domain::UserId{boost::uuids::string_generator{}(
          "7da5fd82-8da1-455f-8c66-0680c821f062")},
      "Hello, Smirkly!",
      std::chrono::system_clock::time_point{
          std::chrono::microseconds{1788436800123456}},
  };
}

UTEST(SendMessageContract, StoresCommandData) {
  const domain::UserId author_id{
      boost::uuids::string_generator{}("7da5fd82-8da1-455f-8c66-0680c821f062")};
  const domain::ConversationId conversation_id{
      boost::uuids::string_generator{}("8d877cac-2aef-4bb4-b48c-766b7f040021")};
  const domain::ClientMessageId client_message_id{
      boost::uuids::string_generator{}("af762c44-1d77-4f02-ad90-4702101de560")};
  const std::string text{"  Hello, Smirkly!  "};

  const SendMessageCommand command{
      .author_id = author_id,
      .conversation_id = conversation_id,
      .client_message_id = client_message_id,
      .text = text,
  };

  EXPECT_EQ(command.author_id, author_id);
  EXPECT_EQ(command.conversation_id, conversation_id);
  EXPECT_EQ(command.client_message_id, client_message_id);
  EXPECT_EQ(command.text, text);
}

UTEST(SendMessageContract, MarksNewlyCreatedMessage) {
  const SendMessageResult result{
      .message = MakeMessage(),
      .was_created = true,
  };

  EXPECT_TRUE(result.was_created);
  EXPECT_EQ(result.message.GetText(), "Hello, Smirkly!");
}

UTEST(SendMessageContract, MarksIdempotentReplay) {
  const SendMessageResult result{
      .message = MakeMessage(),
      .was_created = false,
  };

  EXPECT_FALSE(result.was_created);
  EXPECT_EQ(result.message.GetText(), "Hello, Smirkly!");
}

}  // namespace
}  // namespace smirkly::chat::application::contracts
