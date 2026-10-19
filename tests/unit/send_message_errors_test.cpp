#include <chat/application/errors/send_message.hpp>

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <userver/utest/utest.hpp>

namespace smirkly::chat::application::errors {
namespace {

template <typename Error>
void ExpectRuntimeErrorWithMessage(std::string_view message) {
  static_assert(std::is_base_of_v<std::runtime_error, Error>);

  try {
    throw Error{std::string{message}};
  } catch (const Error& error) {
    EXPECT_EQ(std::string_view{error.what()}, message);
    return;
  }

  FAIL() << "expected application error was not thrown";
}

UTEST(SendMessageErrors, InvalidMessageTextPreservesMessage) {
  ExpectRuntimeErrorWithMessage<InvalidMessageText>("message text is empty");
}

UTEST(SendMessageErrors, MessageTooLongPreservesMessage) {
  ExpectRuntimeErrorWithMessage<MessageTooLong>("message text is too long");
}

UTEST(SendMessageErrors, ConversationAccessDeniedPreservesMessage) {
  ExpectRuntimeErrorWithMessage<ConversationAccessDenied>(
      "author is not a conversation member");
}

UTEST(SendMessageErrors, IdempotencyConflictPreservesMessage) {
  ExpectRuntimeErrorWithMessage<IdempotencyConflict>(
      "client message id already has different data");
}

}  // namespace
}  // namespace smirkly::chat::application::errors
