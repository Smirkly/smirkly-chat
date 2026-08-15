#pragma once

#include <string_view>
#include <userver/server/handlers/http_handler_json_base.hpp>

namespace smirkly::chat::api::health {

class LivenessHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-health-live";

  using HttpHandlerJsonBase::HttpHandlerJsonBase;

  Value HandleRequestJsonThrow(const HttpRequest& request, const Value& body,
                               RequestContext& context) const override;
};

}  // namespace smirkly::chat::api::health
