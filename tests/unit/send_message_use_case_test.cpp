#include <chat/application/usecases/send_message_use_case.hpp>

#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <boost/uuid/string_generator.hpp>
#include <userver/utest/utest.hpp>

#include <chat/application/errors/send_message.hpp>

namespace smirkly::chat::application::usecases {
namespace {

constexpr std::size_t kMaximumTextSize = 4096;

struct TransactionState final {
  std::size_t begin_count{0};
  std::size_t commit_count{0};
  std::size_t rollback_count{0};
  std::optional<std::string> transaction_name;
};

class FakeTransaction final : public ports::DbTransaction {
 public:
  explicit FakeTransaction(TransactionState& state) : state_(state) {}

  ~FakeTransaction() noexcept override {
    if (!committed_) ++state_.rollback_count;
  }

  void Commit() override {
    if (committed_) throw std::logic_error("transaction already committed");
    committed_ = true;
    ++state_.commit_count;
  }

 private:
  TransactionState& state_;
  bool committed_{false};
};

class FakeTransactionManager final : public ports::TransactionManager {
 public:
  [[nodiscard]] std::unique_ptr<ports::DbTransaction> Begin(
      std::string_view transaction_name) override {
    ++state.begin_count;
    state.transaction_name = transaction_name;
    return std::make_unique<FakeTransaction>(state);
  }

  TransactionState state;
};

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

class FakeConversationMemberRepository final
    : public ports::ConversationMemberRepository {
 public:
  bool Exists(ports::DbTransaction& transaction,
              const domain::ConversationId& conversation_id,
              const domain::UserId& user_id) override {
    ++call_count;
    received_transaction = &transaction;
    received_conversation_id = conversation_id;
    received_user_id = user_id;
    if (exception) std::rethrow_exception(exception);
    return exists;
  }

  bool exists{true};
  std::exception_ptr exception;
  std::size_t call_count{0};
  ports::DbTransaction* received_transaction{nullptr};
  std::optional<domain::ConversationId> received_conversation_id;
  std::optional<domain::UserId> received_user_id;
};

class FakeMessageRepository final : public ports::MessageRepository {
 public:
  ports::StoreMessageResult CreateOrGet(
      ports::DbTransaction& transaction,
      const ports::NewMessageData& data) override {
    ++call_count;
    received_transaction = &transaction;
    received_data = data;
    if (exception) std::rethrow_exception(exception);
    if (!next_result) {
      throw std::logic_error("fake message repository result is not set");
    }
    return *next_result;
  }

  std::optional<ports::StoreMessageResult> next_result;
  std::exception_ptr exception;
  std::size_t call_count{0};
  ports::DbTransaction* received_transaction{nullptr};
  std::optional<ports::NewMessageData> received_data;
};

struct TestContext final {
  TestContext() : use_case{transactions, members, messages} {}

  FakeTransactionManager transactions;
  FakeConversationMemberRepository members;
  FakeMessageRepository messages;
  SendMessageUseCase use_case;
};

UTEST(SendMessageUseCase, RejectsEmptyTextWithoutStartingTransaction) {
  TestContext context;
  const auto command = MakeCommand("");

  EXPECT_THROW(context.use_case.Execute(command), errors::InvalidMessageText);
  EXPECT_EQ(context.transactions.state.begin_count, 0);
  EXPECT_EQ(context.members.call_count, 0);
  EXPECT_EQ(context.messages.call_count, 0);
}

UTEST(SendMessageUseCase,
      RejectsTextAboveMaximumLengthWithoutStartingTransaction) {
  TestContext context;
  const auto command = MakeCommand(std::string(kMaximumTextSize + 1, 'a'));

  EXPECT_THROW(context.use_case.Execute(command), errors::MessageTooLong);
  EXPECT_EQ(context.transactions.state.begin_count, 0);
  EXPECT_EQ(context.members.call_count, 0);
  EXPECT_EQ(context.messages.call_count, 0);
}

UTEST(SendMessageUseCase, AcceptsTextAtMaximumLength) {
  TestContext context;
  const auto command = MakeCommand(std::string(kMaximumTextSize, 'a'));
  context.messages.next_result = ports::StoreMessageResult{
      .message = MakeStoredMessage(command),
      .was_created = true,
  };

  const auto result = context.use_case.Execute(command);

  EXPECT_TRUE(result.was_created);
  EXPECT_EQ(result.message.GetText().size(), kMaximumTextSize);
  EXPECT_EQ(context.members.call_count, 1);
  EXPECT_EQ(context.messages.call_count, 1);
  EXPECT_EQ(context.transactions.state.commit_count, 1);
  EXPECT_EQ(context.transactions.state.rollback_count, 0);
}

UTEST(SendMessageUseCase, PassesCommandAndOneTransactionToRepositories) {
  TestContext context;
  const auto command = MakeCommand("  Hello, Smirkly!  ");
  context.messages.next_result = ports::StoreMessageResult{
      .message = MakeStoredMessage(command),
      .was_created = true,
  };

  context.use_case.Execute(command);

  ASSERT_TRUE(context.members.received_conversation_id);
  ASSERT_TRUE(context.members.received_user_id);
  ASSERT_TRUE(context.messages.received_data);
  EXPECT_EQ(*context.members.received_conversation_id, command.conversation_id);
  EXPECT_EQ(*context.members.received_user_id, command.author_id);
  EXPECT_EQ(context.messages.received_data->author_id, command.author_id);
  EXPECT_EQ(context.messages.received_data->conversation_id,
            command.conversation_id);
  EXPECT_EQ(context.messages.received_data->client_message_id,
            command.client_message_id);
  EXPECT_EQ(context.messages.received_data->text, command.text);
  EXPECT_EQ(context.members.received_transaction,
            context.messages.received_transaction);
  ASSERT_TRUE(context.transactions.state.transaction_name);
  EXPECT_EQ(*context.transactions.state.transaction_name, "chat.message.send");
}

UTEST(SendMessageUseCase, RejectsConversationAccessDeniedBeforeStoringMessage) {
  TestContext context;
  context.members.exists = false;
  const auto command = MakeCommand();

  EXPECT_THROW(context.use_case.Execute(command),
               errors::ConversationAccessDenied);
  EXPECT_EQ(context.members.call_count, 1);
  EXPECT_EQ(context.messages.call_count, 0);
  EXPECT_EQ(context.transactions.state.commit_count, 0);
  EXPECT_EQ(context.transactions.state.rollback_count, 1);
}

UTEST(SendMessageUseCase, ReturnsNewlyCreatedMessage) {
  TestContext context;
  const auto command = MakeCommand();
  const auto stored_message = MakeStoredMessage(command);
  context.messages.next_result = ports::StoreMessageResult{
      .message = stored_message,
      .was_created = true,
  };

  const auto result = context.use_case.Execute(command);

  EXPECT_TRUE(result.was_created);
  EXPECT_EQ(result.message.GetId(), stored_message.GetId());
  EXPECT_EQ(result.message.GetClientMessageId(),
            stored_message.GetClientMessageId());
  EXPECT_EQ(result.message.GetConversationId(),
            stored_message.GetConversationId());
  EXPECT_EQ(result.message.GetAuthorId(), stored_message.GetAuthorId());
  EXPECT_EQ(result.message.GetText(), stored_message.GetText());
  EXPECT_EQ(result.message.GetCreatedAt(), stored_message.GetCreatedAt());
  EXPECT_EQ(context.messages.call_count, 1);
  EXPECT_EQ(context.transactions.state.commit_count, 1);
  EXPECT_EQ(context.transactions.state.rollback_count, 0);
}

UTEST(SendMessageUseCase, ReturnsExistingMessageForIdempotentRetry) {
  TestContext context;
  const auto command = MakeCommand();
  const auto stored_message = MakeStoredMessage(command);
  context.messages.next_result = ports::StoreMessageResult{
      .message = stored_message,
      .was_created = false,
  };

  const auto result = context.use_case.Execute(command);

  EXPECT_FALSE(result.was_created);
  EXPECT_EQ(result.message.GetId(), stored_message.GetId());
  EXPECT_EQ(result.message.GetCreatedAt(), stored_message.GetCreatedAt());
  EXPECT_EQ(context.messages.call_count, 1);
  EXPECT_EQ(context.transactions.state.commit_count, 1);
  EXPECT_EQ(context.transactions.state.rollback_count, 0);
}

UTEST(SendMessageUseCase, RejectsRetryWithDifferentText) {
  TestContext context;
  const auto command = MakeCommand("current text");
  context.messages.next_result = ports::StoreMessageResult{
      .message =
          MakeStoredMessage(command, command.conversation_id, "previous text"),
      .was_created = false,
  };

  EXPECT_THROW(context.use_case.Execute(command), errors::IdempotencyConflict);
  EXPECT_EQ(context.transactions.state.commit_count, 0);
  EXPECT_EQ(context.transactions.state.rollback_count, 1);
}

UTEST(SendMessageUseCase, RejectsRetryWithDifferentConversation) {
  TestContext context;
  const auto command = MakeCommand();
  context.messages.next_result = ports::StoreMessageResult{
      .message =
          MakeStoredMessage(command, MakeOtherConversationId(), command.text),
      .was_created = false,
  };

  EXPECT_THROW(context.use_case.Execute(command), errors::IdempotencyConflict);
  EXPECT_EQ(context.transactions.state.commit_count, 0);
  EXPECT_EQ(context.transactions.state.rollback_count, 1);
}

UTEST(SendMessageUseCase, RollsBackWhenCreateOrGetThrows) {
  TestContext context;
  const auto command = MakeCommand();
  context.messages.exception =
      std::make_exception_ptr(std::runtime_error{"database unavailable"});

  EXPECT_THROW(context.use_case.Execute(command), std::runtime_error);
  EXPECT_EQ(context.messages.call_count, 1);
  EXPECT_EQ(context.transactions.state.commit_count, 0);
  EXPECT_EQ(context.transactions.state.rollback_count, 1);
}

}  // namespace
}  // namespace smirkly::chat::application::usecases
