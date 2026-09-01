#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>
#include <openssl/params.h>

#include <chat/platform/auth/jwks_verifier_component.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <userver/clients/http/client.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/formats/json.hpp>
#include <userver/utils/boost_uuid4.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <utility>
#include <vector>

namespace smirkly::chat::platform::auth {
namespace {

using EvpPkeyPtr = std::shared_ptr<EVP_PKEY>;

struct JwtParts final {
  std::string_view header;
  std::string_view payload;
  std::string_view signature;
  std::string_view signing_input;
};

std::vector<unsigned char> Base64UrlDecode(std::string_view encoded) {
  std::string normalized{encoded};
  for (auto& ch : normalized) {
    if (ch == '-') ch = '+';
    if (ch == '_') ch = '/';
  }

  const auto padding = (4 - normalized.size() % 4) % 4;
  normalized.append(padding, '=');
  if (normalized.empty()) return {};

  std::vector<unsigned char> decoded(3 * normalized.size() / 4);
  const auto written = EVP_DecodeBlock(
      decoded.data(), reinterpret_cast<const unsigned char*>(normalized.data()),
      static_cast<int>(normalized.size()));
  if (written < 0 || static_cast<std::size_t>(written) < padding) {
    throw InvalidAccessToken("invalid base64url value");
  }
  decoded.resize(static_cast<std::size_t>(written) - padding);
  return decoded;
}

std::string DecodeText(std::string_view encoded) {
  const auto decoded = Base64UrlDecode(encoded);
  return {reinterpret_cast<const char*>(decoded.data()), decoded.size()};
}

JwtParts SplitJwt(std::string_view token) {
  const auto first = token.find('.');
  const auto second =
      first == std::string_view::npos ? first : token.find('.', first + 1);
  if (first == std::string_view::npos || second == std::string_view::npos ||
      token.find('.', second + 1) != std::string_view::npos || first == 0 ||
      second == first + 1 || second + 1 == token.size()) {
    throw InvalidAccessToken("malformed access token");
  }

  return {
      .header = token.substr(0, first),
      .payload = token.substr(first + 1, second - first - 1),
      .signature = token.substr(second + 1),
      .signing_input = token.substr(0, second),
  };
}

EvpPkeyPtr BuildRsaPublicKey(std::string_view modulus,
                             std::string_view exponent) {
  const auto n_bytes = Base64UrlDecode(modulus);
  const auto e_bytes = Base64UrlDecode(exponent);
  if (n_bytes.empty() || e_bytes.empty()) {
    throw JwksUnavailable("JWKS contains an empty RSA key");
  }

  using BnPtr = std::unique_ptr<BIGNUM, decltype(&BN_free)>;
  using ParamBldPtr =
      std::unique_ptr<OSSL_PARAM_BLD, decltype(&OSSL_PARAM_BLD_free)>;
  using ParamsPtr = std::unique_ptr<OSSL_PARAM, decltype(&OSSL_PARAM_free)>;
  using PkeyCtxPtr =
      std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;

  BnPtr n(BN_bin2bn(n_bytes.data(), static_cast<int>(n_bytes.size()), nullptr),
          BN_free);
  BnPtr e(BN_bin2bn(e_bytes.data(), static_cast<int>(e_bytes.size()), nullptr),
          BN_free);
  ParamBldPtr builder(OSSL_PARAM_BLD_new(), OSSL_PARAM_BLD_free);
  PkeyCtxPtr context(EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr),
                     EVP_PKEY_CTX_free);
  if (!n || !e || !builder || !context ||
      OSSL_PARAM_BLD_push_BN(builder.get(), OSSL_PKEY_PARAM_RSA_N, n.get()) !=
          1 ||
      OSSL_PARAM_BLD_push_BN(builder.get(), OSSL_PKEY_PARAM_RSA_E, e.get()) !=
          1) {
    throw JwksUnavailable("failed to prepare RSA key from JWKS");
  }

  ParamsPtr params(OSSL_PARAM_BLD_to_param(builder.get()), OSSL_PARAM_free);
  EVP_PKEY* raw_key = nullptr;
  if (!params || EVP_PKEY_fromdata_init(context.get()) != 1 ||
      EVP_PKEY_fromdata(context.get(), &raw_key, EVP_PKEY_PUBLIC_KEY,
                        params.get()) != 1 ||
      raw_key == nullptr) {
    if (raw_key) EVP_PKEY_free(raw_key);
    throw JwksUnavailable("failed to build RSA key from JWKS");
  }

  return EvpPkeyPtr(raw_key, EVP_PKEY_free);
}

void VerifyRs256(const JwtParts& parts, const EVP_PKEY& key) {
  using MdCtxPtr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  MdCtxPtr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  const auto signature = Base64UrlDecode(parts.signature);
  if (!context || signature.empty() ||
      EVP_DigestVerifyInit(context.get(), nullptr, EVP_sha256(), nullptr,
                           const_cast<EVP_PKEY*>(&key)) != 1 ||
      EVP_DigestVerifyUpdate(context.get(), parts.signing_input.data(),
                             parts.signing_input.size()) != 1 ||
      EVP_DigestVerifyFinal(context.get(), signature.data(),
                            signature.size()) != 1) {
    throw InvalidAccessToken("invalid access token signature");
  }
}

bool AudienceMatches(const userver::formats::json::Value& audience,
                     std::string_view expected) {
  if (audience.IsString()) {
    return audience.As<std::string>() == expected;
  }
  if (audience.IsArray()) {
    for (const auto& item : audience) {
      if (item.IsString() && item.As<std::string>() == expected) return true;
    }
  }
  return false;
}

std::string CanonicalUuid(const userver::formats::json::Value& value) {
  return userver::utils::ToString(
      userver::utils::BoostUuidFromString(value.As<std::string>()));
}

}  // namespace

class JwksVerifierComponent::Impl final {
 public:
  Impl(const userver::components::ComponentConfig& config,
       const userver::components::ComponentContext& context)
      : http_client_(context.FindComponent<userver::components::HttpClient>()
                         .GetHttpClient()),
        jwks_url_(config["jwks-url"].As<std::string>()),
        issuer_(config["issuer"].As<std::string>()),
        audience_(config["audience"].As<std::string>()),
        cache_ttl_(config["cache-ttl-seconds"].As<int>()),
        request_timeout_(config["request-timeout-ms"].As<int>()) {}

  AccessTokenClaims Verify(std::string_view token) const {
    try {
      const auto parts = SplitJwt(token);
      const auto header =
          userver::formats::json::FromString(DecodeText(parts.header));
      if (header["alg"].As<std::string>() != "RS256") {
        throw InvalidAccessToken("unsupported access token algorithm");
      }
      const auto key_id = header["kid"].As<std::string>();
      const auto key = GetKey(key_id);
      VerifyRs256(parts, *key);

      const auto payload =
          userver::formats::json::FromString(DecodeText(parts.payload));
      if (payload["iss"].As<std::string>() != issuer_ ||
          !AudienceMatches(payload["aud"], audience_) ||
          payload["type"].As<std::string>() != "access") {
        throw InvalidAccessToken("invalid access token claims");
      }

      const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
      if (payload["exp"].As<std::int64_t>() <= now) {
        throw InvalidAccessToken("access token expired");
      }
      if (!payload["nbf"].IsMissing() &&
          payload["nbf"].As<std::int64_t>() > now) {
        throw InvalidAccessToken("access token is not active");
      }

      return {
          .user_id = CanonicalUuid(payload["sub"]),
          .session_id = CanonicalUuid(payload["sid"]),
      };
    } catch (const InvalidAccessToken&) {
      throw;
    } catch (const JwksUnavailable&) {
      throw;
    } catch (const std::exception&) {
      throw InvalidAccessToken("invalid access token");
    }
  }

 private:
  EvpPkeyPtr GetKey(const std::string& key_id) const {
    std::unique_lock lock(mutex_);
    const auto now = std::chrono::steady_clock::now();
    auto it = keys_.find(key_id);
    if (it == keys_.end() || now >= expires_at_) {
      RefreshKeys();
      it = keys_.find(key_id);
    }
    if (it == keys_.end()) {
      throw InvalidAccessToken("unknown access token key id");
    }
    return it->second;
  }

  void RefreshKeys() const {
    try {
      const auto response = http_client_.CreateRequest()
                                .get(jwks_url_)
                                .retry(2)
                                .timeout(request_timeout_)
                                .perform();
      response->raise_for_status();
      const auto document =
          userver::formats::json::FromString(response->body_view());

      std::unordered_map<std::string, EvpPkeyPtr> refreshed;
      for (const auto& jwk : document["keys"]) {
        if (jwk["kty"].As<std::string>() != "RSA" ||
            jwk["alg"].As<std::string>() != "RS256") {
          continue;
        }
        refreshed.emplace(jwk["kid"].As<std::string>(),
                          BuildRsaPublicKey(jwk["n"].As<std::string>(),
                                            jwk["e"].As<std::string>()));
      }
      if (refreshed.empty()) {
        throw JwksUnavailable("JWKS contains no supported signing keys");
      }

      keys_ = std::move(refreshed);
      expires_at_ = std::chrono::steady_clock::now() + cache_ttl_;
    } catch (const JwksUnavailable&) {
      throw;
    } catch (const std::exception& error) {
      throw JwksUnavailable(std::string{"failed to refresh JWKS: "} +
                            error.what());
    }
  }

  userver::clients::http::Client& http_client_;
  std::string jwks_url_;
  std::string issuer_;
  std::string audience_;
  std::chrono::seconds cache_ttl_;
  std::chrono::milliseconds request_timeout_;

  mutable userver::engine::Mutex mutex_;
  mutable std::unordered_map<std::string, EvpPkeyPtr> keys_;
  mutable std::chrono::steady_clock::time_point expires_at_{};
};

JwksVerifierComponent::JwksVerifierComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      impl_(std::make_unique<Impl>(config, context)) {}

JwksVerifierComponent::~JwksVerifierComponent() = default;

AccessTokenClaims JwksVerifierComponent::Verify(std::string_view token) const {
  return impl_->Verify(token);
}

userver::yaml_config::Schema JwksVerifierComponent::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: Validates Smirkly access JWTs against the auth service JWKS
additionalProperties: false
properties:
    jwks-url:
        type: string
        description: URL of the smirkly-auth JSON Web Key Set endpoint
    issuer:
        type: string
        description: Expected access token issuer claim
    audience:
        type: string
        description: Expected access token audience claim
    cache-ttl-seconds:
        type: integer
        description: Successful JWKS response cache lifetime in seconds
        minimum: 1
        default: 300
    request-timeout-ms:
        type: integer
        description: Timeout for a JWKS HTTP request in milliseconds
        minimum: 1
        default: 2000
)");
}

}  // namespace smirkly::chat::platform::auth
