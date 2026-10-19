#include <type_traits>

#include <boost/uuid/string_generator.hpp>
#include <userver/utest/utest.hpp>

#include <chat/domain/types/client_message_id.hpp>
#include <chat/domain/types/conversation_id.hpp>
#include <chat/domain/types/message_id.hpp>
#include <chat/domain/types/user_id.hpp>

namespace smirkly::chat::domain {
namespace {

UTEST(DomainTypes, ComparesUserIdsByValue) {
  const auto value =
      boost::uuids::string_generator{}("5f648f9e-4450-4d8d-b3d8-1e5612ccdb48");
  const auto other_value =
      boost::uuids::string_generator{}("b5629270-bce7-407d-80de-0099a599f778");

  const UserId first{value};
  const UserId same{value};
  const UserId other{other_value};

  EXPECT_EQ(first, same);
  EXPECT_NE(first, other);
  EXPECT_EQ(first.Value(), value);
}

UTEST(DomainTypes, ComparesConversationIdsByValue) {
  const auto value =
      boost::uuids::string_generator{}("b5306e95-ef27-4e76-bffa-98f79ed972fc");
  const auto other_value =
      boost::uuids::string_generator{}("eb149bee-a7b0-4e5f-a7f1-23f3b7658a64");

  const ConversationId first{value};
  const ConversationId same{value};
  const ConversationId other{other_value};

  EXPECT_EQ(first, same);
  EXPECT_NE(first, other);
  EXPECT_EQ(first.Value(), value);
}

UTEST(DomainTypes, ComparesMessageIdsByValue) {
  const auto value =
      boost::uuids::string_generator{}("d275c17c-8880-481a-a33a-3803a72ad035");
  const auto other_value =
      boost::uuids::string_generator{}("4e708172-f5ce-48d9-bd21-cd6f58221c11");

  const MessageId first{value};
  const MessageId same{value};
  const MessageId other{other_value};

  EXPECT_EQ(first, same);
  EXPECT_NE(first, other);
  EXPECT_EQ(first.Value(), value);
}

UTEST(DomainTypes, ComparesClientMessageIdsByValue) {
  const auto value =
      boost::uuids::string_generator{}("52906bb8-7e25-48a7-a26c-538821645373");
  const auto other_value =
      boost::uuids::string_generator{}("d8407605-f6aa-4543-a205-c46880d934ab");

  const ClientMessageId first{value};
  const ClientMessageId same{value};
  const ClientMessageId other{other_value};

  EXPECT_EQ(first, same);
  EXPECT_NE(first, other);
  EXPECT_EQ(first.Value(), value);
}

UTEST(DomainTypes, KeepsIdentifierTypesDistinct) {
  static_assert(!std::is_same_v<UserId, ConversationId>);
  static_assert(!std::is_same_v<UserId, MessageId>);
  static_assert(!std::is_same_v<UserId, ClientMessageId>);
  static_assert(!std::is_same_v<ConversationId, MessageId>);
  static_assert(!std::is_same_v<ConversationId, ClientMessageId>);
  static_assert(!std::is_same_v<MessageId, ClientMessageId>);
}

UTEST(DomainTypes, RequiresExplicitUuidConstruction) {
  static_assert(std::is_constructible_v<UserId, boost::uuids::uuid>);
  static_assert(std::is_constructible_v<ConversationId, boost::uuids::uuid>);
  static_assert(std::is_constructible_v<MessageId, boost::uuids::uuid>);
  static_assert(std::is_constructible_v<ClientMessageId, boost::uuids::uuid>);

  static_assert(!std::is_convertible_v<boost::uuids::uuid, UserId>);
  static_assert(!std::is_convertible_v<boost::uuids::uuid, ConversationId>);
  static_assert(!std::is_convertible_v<boost::uuids::uuid, MessageId>);
  static_assert(!std::is_convertible_v<boost::uuids::uuid, ClientMessageId>);
}

UTEST(DomainTypes, RequiresUuidForConstruction) {
  static_assert(!std::is_default_constructible_v<UserId>);
  static_assert(!std::is_default_constructible_v<ConversationId>);
  static_assert(!std::is_default_constructible_v<MessageId>);
  static_assert(!std::is_default_constructible_v<ClientMessageId>);
}

}  // namespace
}  // namespace smirkly::chat::domain
