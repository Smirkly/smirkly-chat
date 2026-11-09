#include <chat/application/delivery/connection_registry.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace smirkly::chat::application::delivery {
namespace {

TEST(OutboundMailboxTest, PreservesFifoAndRejectsOverflow) {
  OutboundMailbox mailbox{2};
  EXPECT_TRUE(mailbox.Push("first"));
  EXPECT_TRUE(mailbox.Push("second"));
  EXPECT_FALSE(mailbox.Push("third"));

  std::string event;
  ASSERT_TRUE(mailbox.TryPop(event));
  EXPECT_EQ(event, "first");
  ASSERT_TRUE(mailbox.TryPop(event));
  EXPECT_EQ(event, "second");
  EXPECT_FALSE(mailbox.TryPop(event));
}

TEST(OutboundMailboxTest, ClosedMailboxRejectsNewEvents) {
  OutboundMailbox mailbox;
  mailbox.Push("pending");
  mailbox.Close();

  std::string event;
  EXPECT_FALSE(mailbox.Push("after-close"));
  EXPECT_FALSE(mailbox.TryPop(event));
}

TEST(ConnectionRegistryTest, PublishesToEveryDeviceExceptOrigin) {
  ConnectionRegistry registry;
  auto sender = registry.Register("user-a", "a-1");
  auto first_device = registry.Register("user-b", "b-1");
  auto second_device = registry.Register("user-b", "b-2");

  EXPECT_EQ(registry.Publish({"user-a", "user-b"}, "a-1", "message.new"),
            2U);

  std::string event;
  EXPECT_FALSE(sender->TryPop(event));
  ASSERT_TRUE(first_device->TryPop(event));
  EXPECT_EQ(event, "message.new");
  ASSERT_TRUE(second_device->TryPop(event));
  EXPECT_EQ(event, "message.new");
}

TEST(ConnectionRegistryTest, ReplacingConnectionClosesOldMailbox) {
  ConnectionRegistry registry;
  auto old_mailbox = registry.Register("user-a", "same-id");
  auto new_mailbox = registry.Register("user-a", "same-id");

  EXPECT_FALSE(old_mailbox->Push("stale"));
  EXPECT_TRUE(new_mailbox->Push("current"));
}

TEST(ConnectionRegistryTest, DuplicateRecipientIdsDoNotDuplicateEvents) {
  ConnectionRegistry registry;
  auto mailbox = registry.Register("user-b", "b-1");

  EXPECT_EQ(registry.Publish({"user-b", "user-b"}, "sender", "message.new"),
            1U);
  std::string event;
  ASSERT_TRUE(mailbox->TryPop(event));
  EXPECT_EQ(event, "message.new");
  EXPECT_FALSE(mailbox->TryPop(event));
}

TEST(ConnectionRegistryTest, UnregisterRemovesOnlySelectedConnection) {
  ConnectionRegistry registry;
  auto first = registry.Register("user-b", "b-1");
  auto second = registry.Register("user-b", "b-2");

  registry.Unregister("user-b", "b-1");
  EXPECT_FALSE(first->Push("message.new"));
  EXPECT_EQ(registry.Publish({"user-b"}, "sender", "message.new"), 1U);

  std::string event;
  ASSERT_TRUE(second->TryPop(event));
  EXPECT_EQ(event, "message.new");
}

}  // namespace
}  // namespace smirkly::chat::application::delivery
