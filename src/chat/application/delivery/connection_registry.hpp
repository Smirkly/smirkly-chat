#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace smirkly::chat::application::delivery {

// A per-WebSocket outbound mailbox. Producers only enqueue; the socket-owning
// handler is the sole consumer and remains the only code that writes to socket.
class OutboundMailbox final {
 public:
  explicit OutboundMailbox(std::size_t capacity = 256) : capacity_(capacity) {}

  bool Push(std::string event);
  bool TryPop(std::string& event);
  void Close();

 private:
  const std::size_t capacity_;
  std::mutex mutex_;
  std::deque<std::string> events_;
  bool closed_{false};
};

// Keeps weakly-coupled mailboxes rather than WebSocket references. A user may
// have multiple active devices, each with its own mailbox.
class ConnectionRegistry final {
 public:
  using MailboxPtr = std::shared_ptr<OutboundMailbox>;

  MailboxPtr Register(std::string user_id, std::string connection_id);
  void Unregister(std::string_view user_id, std::string_view connection_id);
  std::size_t Publish(const std::vector<std::string>& recipient_user_ids,
                      std::string_view originating_connection_id,
                      const std::string& event);

 private:
  struct Entry final {
    std::string connection_id;
    MailboxPtr mailbox;
  };

  std::mutex mutex_;
  std::unordered_map<std::string, std::vector<Entry>> connections_;
};

}  // namespace smirkly::chat::application::delivery
