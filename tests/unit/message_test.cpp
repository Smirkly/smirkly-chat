#include <chrono>
#include <string>
#include <type_traits>

#include <boost/uuid/string_generator.hpp>
#include <userver/utest/utest.hpp>

#include <chat/domain/message.hpp>

namespace smirkly::chat::domain {
namespace {

UTEST(Message, StoresPersistedState) {
  const MessageId id{
      boost::uuids::string_generator{}("85e97f15-dd0f-4d26-a57b-0ea8164d2139")};
  const ClientMessageId client_message_id{
      boost::uuids::string_generator{}("af762c44-1d77-4f02-ad90-4702101de560")};
  const ConversationId conversation_id{
      boost::uuids::string_generator{}("8d877cac-2aef-4bb4-b48c-766b7f040021")};
  const UserId author_id{
      boost::uuids::string_generator{}("7da5fd82-8da1-455f-8c66-0680c821f062")};
  const std::string text{"  Hello, Smirkly!  "};
  const auto created_at = std::chrono::system_clock::time_point{
      std::chrono::microseconds{1788436800123456}};

  const Message message{id,   client_message_id, conversation_id, author_id,
                        text, created_at};

  EXPECT_EQ(message.GetId(), id);
  EXPECT_EQ(message.GetClientMessageId(), client_message_id);
  EXPECT_EQ(message.GetConversationId(), conversation_id);
  EXPECT_EQ(message.GetAuthorId(), author_id);
  EXPECT_EQ(message.GetText(), text);
  EXPECT_EQ(message.GetCreatedAt(), created_at);
}

UTEST(Message, RequiresAllFields) {
  static_assert(!std::is_default_constructible_v<Message>);
}

UTEST(Message, ComparesByMessageId) {
  const MessageId shared_id{
      boost::uuids::string_generator{}("741e4229-2472-4bee-a3b4-973bf92ab61d")};
  const MessageId other_id{
      boost::uuids::string_generator{}("7a7821ee-7185-4599-b75f-92b0999eb28b")};
  const ClientMessageId first_client_id{
      boost::uuids::string_generator{}("59680cc9-aa0a-4715-9d10-8bdf8dbf70aa")};
  const ClientMessageId second_client_id{
      boost::uuids::string_generator{}("ce4d7827-75b5-4344-9be7-256b18985fd8")};
  const ConversationId first_conversation_id{
      boost::uuids::string_generator{}("318d0f21-cdd8-4ca4-9c55-a799c016b6e4")};
  const ConversationId second_conversation_id{
      boost::uuids::string_generator{}("f003c916-ad69-4455-ab22-c3bc46d099ab")};
  const UserId first_author_id{
      boost::uuids::string_generator{}("bf8f57c8-08bd-498e-a57b-d5cd7c28467e")};
  const UserId second_author_id{
      boost::uuids::string_generator{}("dc7f2948-d2ab-4b51-96e4-b901c399e26d")};
  const auto created_at = std::chrono::system_clock::time_point{
      std::chrono::microseconds{1788436800123456}};

  const Message first{shared_id,       first_client_id, first_conversation_id,
                      first_author_id, "first",         created_at};
  const Message same_identity{
      shared_id,        second_client_id, second_conversation_id,
      second_author_id, "different text", created_at + std::chrono::seconds{1}};
  const Message different_identity{
      other_id,        first_client_id, first_conversation_id,
      first_author_id, "first",         created_at};

  EXPECT_EQ(first, same_identity);
  EXPECT_NE(first, different_identity);
}

}  // namespace
}  // namespace smirkly::chat::domain
