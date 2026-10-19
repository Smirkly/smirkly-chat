#include <chat/infra/postgres/mappers/message_mapper.hpp>

#include <chrono>
#include <string>
#include <utility>

#include <boost/uuid/string_generator.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/utest/utest.hpp>

namespace smirkly::chat::infra::postgres::mappers {
namespace {

UTEST(MessageMapper, MapsPostgresRowToDomainMessage) {
  const auto id =
      boost::uuids::string_generator{}("85e97f15-dd0f-4d26-a57b-0ea8164d2139");
  const auto client_message_id =
      boost::uuids::string_generator{}("af762c44-1d77-4f02-ad90-4702101de560");
  const auto conversation_id =
      boost::uuids::string_generator{}("8d877cac-2aef-4bb4-b48c-766b7f040021");
  const auto author_id =
      boost::uuids::string_generator{}("7da5fd82-8da1-455f-8c66-0680c821f062");
  const std::string text{"Hello, Smirkly!"};
  const auto created_at = std::chrono::system_clock::time_point{
      std::chrono::microseconds{1788436800123456}};

  types::MessageRow row{
      .id = id,
      .client_message_id = client_message_id,
      .conversation_id = conversation_id,
      .author_id = author_id,
      .text = text,
      .created_at =
          USERVER_NAMESPACE::storages::postgres::TimePointTz{created_at},
  };

  const auto message = ToDomain(std::move(row));

  EXPECT_EQ(message.GetId().Value(), id);
  EXPECT_EQ(message.GetClientMessageId().Value(), client_message_id);
  EXPECT_EQ(message.GetConversationId().Value(), conversation_id);
  EXPECT_EQ(message.GetAuthorId().Value(), author_id);
  EXPECT_EQ(message.GetText(), text);
  EXPECT_EQ(message.GetCreatedAt(), created_at);
}

}  // namespace
}  // namespace smirkly::chat::infra::postgres::mappers
