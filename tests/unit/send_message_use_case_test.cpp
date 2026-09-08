#include <chat/application/usecases/send_message_use_case.hpp>

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>

#include <boost/uuid/string_generator.hpp>
#include <userver/utest/utest.hpp>

#include <chat/application/errors/send_message.hpp>

namespace smirkly::chat::application::usecases {
namespace {

constexpr std::size_t kMaximumTextSize = 4096;

domain::UserId MakeAuthorId() {
  return domain::UserId{
      boost::uuids::string_generator{}("7da5fd82-8da1-455f-8c66-0680c821f062")};
}

domain::ConversationId MakeConversationId() {
  return domain::ConversationId{
      boost::uuids::string_generator{}("8d877cac-2aef-4bb4-b48c-766b7f040021")};
}

domain::ConversationId MakeOtherConversationId() {
  return domain::ConversationId{
      boost::uuids::string_generator{}("47c2a3bd-e5aa-40b5-9832-8581287a8c69")};
}

domain::ClientMessageId MakeClientMessageId() {
  return domain::ClientMessageId{
      boost::uuids::string_generator{}("af762c44-1d77-4f02-ad90-4702101de560")};
}

domain::MessageId MakeMessageId() {
  return domain::MessageId{
      boost::uuids::string_generator{}("85e97f15-dd0f-4d26-a57b-0ea8164d2139")};
}

std::chrono::system_clock::time_point MakeCreatedAt() {
  return std::chrono::system_clock::time_point{
      std::chrono::microseconds{1788436800123456}};
}

contracts::SendMessageCommand MakeCommand(std::string text = "Hello") {
  return {
      .author_id = MakeAuthorId(),
      .conversation_id = MakeConversationId(),
      .client_message_id = MakeClientMessageId(),
      .text = std::move(text),
  };
}

domain::Message MakeStoredMessage(
    const contracts::SendMessageCommand& command) {
  return {
      MakeMessageId(),
      command.client_message_id,
      command.conversation_id,
      command.author_id,
      command.text,
      MakeCreatedAt(),
  };
}

domain::Message MakeStoredMessage(const contracts::SendMessageCommand& command,
                                  domain::ConversationId conversation_id,
                                  std::string text) {
  return {
      MakeMessageId(),   command.client_message_id, std::move(conversation_id),
      command.author_id, std::move(text),           MakeCreatedAt(),
  };
}

class FakeMessageRepository final : public ports::MessageRepository {
 public:
  std::optional<ports::StoreMessageResult> CreateOrGetIfMember(
      const domain::UserId& author_id,
      const domain::ConversationId& conversation_id,
      const domain::ClientMessageId& client_message_id,
      const std::string& text) override {
    ++call_count;
    received_author_id = author_id;
    received_conversation_id = conversation_id;
    received_client_message_id = client_message_id;
    received_text = text;
    return next_result;
  }

  std::optional<ports::StoreMessageResult> next_result;
  std::size_t call_count{0};
  std::optional<domain::UserId> received_author_id;
  std::optional<domain::ConversationId> received_conversation_id;
  std::optional<domain::ClientMessageId> received_client_message_id;
  std::optional<std::string> received_text;
};

UTEST(SendMessageUseCase, RejectsEmptyTextWithoutCallingRepository) {
  FakeMessageRepository messages;
  const SendMessageUseCase use_case{messages};
  const auto command = MakeCommand("");

  EXPECT_THROW(use_case.Execute(command), errors::InvalidMessageText);
  EXPECT_EQ(messages.call_count, 0);
}

UTEST(SendMessageUseCase,
      RejectsTextAboveMaximumLengthWithoutCallingRepository) {
  FakeMessageRepository messages;
  const SendMessageUseCase use_case{messages};
  const auto command = MakeCommand(std::string(kMaximumTextSize + 1, 'a'));

  EXPECT_THROW(use_case.Execute(command), errors::MessageTooLong);
  EXPECT_EQ(messages.call_count, 0);
}

UTEST(SendMessageUseCase, AcceptsTextAtMaximumLength) {
  FakeMessageRepository messages;
  const SendMessageUseCase use_case{messages};
  const auto command = MakeCommand(std::string(kMaximumTextSize, 'a'));
  messages.next_result = ports::StoreMessageResult{
      .message = MakeStoredMessage(command),
      .was_created = true,
  };

  const auto result = use_case.Execute(command);

  EXPECT_TRUE(result.was_created);
  EXPECT_EQ(result.message.GetText().size(), kMaximumTextSize);
  EXPECT_EQ(messages.call_count, 1);
}

UTEST(SendMessageUseCase, PassesCommandToRepositoryWithoutChangingText) {
  FakeMessageRepository messages;
  const SendMessageUseCase use_case{messages};
  const auto command = MakeCommand("  Hello, Smirkly!  ");
  messages.next_result = ports::StoreMessageResult{
      .message = MakeStoredMessage(command),
      .was_created = true,
  };

  use_case.Execute(command);

  ASSERT_TRUE(messages.received_author_id);
  ASSERT_TRUE(messages.received_conversation_id);
  ASSERT_TRUE(messages.received_client_message_id);
  ASSERT_TRUE(messages.received_text);
  EXPECT_EQ(*messages.received_author_id, command.author_id);
  EXPECT_EQ(*messages.received_conversation_id, command.conversation_id);
  EXPECT_EQ(*messages.received_client_message_id, command.client_message_id);
  EXPECT_EQ(*messages.received_text, command.text);
}

UTEST(SendMessageUseCase, RejectsConversationAccessDenied) {
  FakeMessageRepository messages;
  const SendMessageUseCase use_case{messages};
  const auto command = MakeCommand();

  EXPECT_THROW(use_case.Execute(command), errors::ConversationAccessDenied);
  EXPECT_EQ(messages.call_count, 1);
}

UTEST(SendMessageUseCase, ReturnsNewlyCreatedMessage) {
  FakeMessageRepository messages;
  const SendMessageUseCase use_case{messages};
  const auto command = MakeCommand();
  const auto stored_message = MakeStoredMessage(command);
  messages.next_result = ports::StoreMessageResult{
      .message = stored_message,
      .was_created = true,
  };

  const auto result = use_case.Execute(command);

  EXPECT_TRUE(result.was_created);
  EXPECT_EQ(result.message.GetId(), stored_message.GetId());
  EXPECT_EQ(result.message.GetClientMessageId(),
            stored_message.GetClientMessageId());
  EXPECT_EQ(result.message.GetConversationId(),
            stored_message.GetConversationId());
  EXPECT_EQ(result.message.GetAuthorId(), stored_message.GetAuthorId());
  EXPECT_EQ(result.message.GetText(), stored_message.GetText());
  EXPECT_EQ(result.message.GetCreatedAt(), stored_message.GetCreatedAt());
}

UTEST(SendMessageUseCase, ReturnsExistingMessageForIdempotentRetry) {
  FakeMessageRepository messages;
  const SendMessageUseCase use_case{messages};
  const auto command = MakeCommand();
  const auto stored_message = MakeStoredMessage(command);
  messages.next_result = ports::StoreMessageResult{
      .message = stored_message,
      .was_created = false,
  };

  const auto result = use_case.Execute(command);

  EXPECT_FALSE(result.was_created);
  EXPECT_EQ(result.message.GetId(), stored_message.GetId());
  EXPECT_EQ(result.message.GetCreatedAt(), stored_message.GetCreatedAt());
}

UTEST(SendMessageUseCase, RejectsRetryWithDifferentText) {
  FakeMessageRepository messages;
  const SendMessageUseCase use_case{messages};
  const auto command = MakeCommand("current text");
  messages.next_result = ports::StoreMessageResult{
      .message =
          MakeStoredMessage(command, command.conversation_id, "previous text"),
      .was_created = false,
  };

  EXPECT_THROW(use_case.Execute(command), errors::IdempotencyConflict);
}

UTEST(SendMessageUseCase, RejectsRetryWithDifferentConversation) {
  FakeMessageRepository messages;
  const SendMessageUseCase use_case{messages};
  const auto command = MakeCommand();
  messages.next_result = ports::StoreMessageResult{
      .message =
          MakeStoredMessage(command, MakeOtherConversationId(), command.text),
      .was_created = false,
  };

  EXPECT_THROW(use_case.Execute(command), errors::IdempotencyConflict);
}

}  // namespace
}  // namespace smirkly::chat::application::usecases
