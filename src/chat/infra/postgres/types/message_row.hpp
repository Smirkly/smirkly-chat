#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/uuid.hpp>

namespace smirkly::chat::infra::postgres::types {

// Field order must match the SELECT and RETURNING lists of message queries.
struct MessageRow final {
  boost::uuids::uuid id;
  boost::uuids::uuid client_message_id;
  boost::uuids::uuid conversation_id;
  boost::uuids::uuid author_id;
  std::string text;
  USERVER_NAMESPACE::storages::postgres::TimePointTz created_at;
};

}  // namespace smirkly::chat::infra::postgres::types
