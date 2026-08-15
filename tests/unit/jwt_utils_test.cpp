#include <chat/platform/auth/bearer_token.hpp>
#include <userver/utest/utest.hpp>

namespace {

UTEST(BearerToken, ExtractsValidToken) {
  const auto token = smirkly::chat::platform::auth::ExtractBearerToken("Bearer abc.def.ghi");
  ASSERT_TRUE(token);
  EXPECT_EQ(*token, "abc.def.ghi");
}

UTEST(BearerToken, RejectsMalformedHeader) {
  using smirkly::chat::platform::auth::ExtractBearerToken;
  EXPECT_FALSE(ExtractBearerToken(""));
  EXPECT_FALSE(ExtractBearerToken("Basic abc"));
  EXPECT_FALSE(ExtractBearerToken("Bearer "));
  EXPECT_FALSE(ExtractBearerToken("Bearer token with spaces"));
  EXPECT_FALSE(ExtractBearerToken("bearer abc"));
}

}  // namespace
