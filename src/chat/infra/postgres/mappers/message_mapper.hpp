#pragma once

#include <chat/domain/message.hpp>
#include <chat/infra/postgres/types/message_row.hpp>

namespace smirkly::chat::infra::postgres::mappers {

[[nodiscard]] domain::Message ToDomain(types::MessageRow row);

}  // namespace smirkly::chat::infra::postgres::mappers
