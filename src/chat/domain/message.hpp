#pragma once

#include <chrono>
#include <string>
#include <utility>

#include <chat/domain/types/client_message_id.hpp>
#include <chat/domain/types/conversation_id.hpp>
#include <chat/domain/types/message_id.hpp>
#include <chat/domain/types/user_id.hpp>

namespace smirkly::chat::domain {
class Message final {
 public:
  Message(MessageId id, ClientMessageId client_message_id,
          ConversationId conversation_id, UserId author_id, std::string text,
          std::chrono::system_clock::time_point created_at)
      : id_(id),
        client_message_id_(client_message_id),
        conversation_id_(conversation_id),
        author_id_(author_id),
        text_(std::move(text)),
        created_at_(created_at) {}

  friend bool operator==(const Message& lhs, const Message& rhs) noexcept {
    return lhs.id_ == rhs.id_;
  }

  const MessageId& GetId() const noexcept { return id_; }
  const ClientMessageId& GetClientMessageId() const noexcept {
    return client_message_id_;
  }
  const ConversationId& GetConversationId() const noexcept {
    return conversation_id_;
  }
  const UserId& GetAuthorId() const noexcept { return author_id_; }
  const std::string& GetText() const noexcept { return text_; }
  std::chrono::system_clock::time_point GetCreatedAt() const noexcept {
    return created_at_;
  }

 private:
  MessageId id_;
  ClientMessageId client_message_id_;
  ConversationId conversation_id_;
  UserId author_id_;
  std::string text_;
  std::chrono::system_clock::time_point created_at_;
};
}  // namespace smirkly::chat::domain