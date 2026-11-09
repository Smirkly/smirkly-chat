#include <chat/application/delivery/connection_registry.hpp>

#include <algorithm>
#include <utility>

namespace smirkly::chat::application::delivery {

bool OutboundMailbox::Push(std::string event) {
  std::lock_guard lock(mutex_);
  if (closed_ || events_.size() >= capacity_) return false;
  events_.push_back(std::move(event));
  return true;
}

bool OutboundMailbox::TryPop(std::string& event) {
  std::lock_guard lock(mutex_);
  if (events_.empty()) return false;
  event = std::move(events_.front());
  events_.pop_front();
  return true;
}

void OutboundMailbox::Close() {
  std::lock_guard lock(mutex_);
  closed_ = true;
  events_.clear();
}

ConnectionRegistry::MailboxPtr ConnectionRegistry::Register(
    std::string user_id, std::string connection_id) {
  auto mailbox = std::make_shared<OutboundMailbox>();
  std::lock_guard lock(mutex_);
  auto& entries = connections_[std::move(user_id)];
  entries.erase(std::remove_if(entries.begin(), entries.end(),
                               [&connection_id](const Entry& entry) {
                                 return entry.connection_id == connection_id;
                               }),
                entries.end());
  entries.push_back(Entry{std::move(connection_id), mailbox});
  return mailbox;
}

void ConnectionRegistry::Unregister(std::string_view user_id,
                                    std::string_view connection_id) {
  std::lock_guard lock(mutex_);
  const auto it = connections_.find(std::string{user_id});
  if (it == connections_.end()) return;

  auto& entries = it->second;
  for (auto entry = entries.begin(); entry != entries.end();) {
    if (entry->connection_id == connection_id) {
      entry->mailbox->Close();
      entry = entries.erase(entry);
    } else {
      ++entry;
    }
  }
  if (entries.empty()) connections_.erase(it);
}

std::size_t ConnectionRegistry::Publish(
    const std::vector<std::string>& recipient_user_ids,
    std::string_view originating_connection_id, const std::string& event) {
  std::vector<MailboxPtr> recipients;
  {
    std::lock_guard lock(mutex_);
    for (const auto& user_id : recipient_user_ids) {
      const auto it = connections_.find(user_id);
      if (it == connections_.end()) continue;
      for (const auto& entry : it->second) {
        if (entry.connection_id != originating_connection_id) {
          recipients.push_back(entry.mailbox);
        }
      }
    }
  }

  std::size_t queued = 0;
  for (const auto& mailbox : recipients) {
    if (mailbox->Push(event)) ++queued;
  }
  return queued;
}

}  // namespace smirkly::chat::application::delivery
