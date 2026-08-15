#include <chat/api/health/liveness_handler.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace smirkly::chat::api::health {

LivenessHandler::Value LivenessHandler::HandleRequestJsonThrow(const HttpRequest&, const Value&,
                                                               RequestContext&) const {
  userver::formats::json::ValueBuilder response;
  response["status"] = "ok";
  return response.ExtractValue();
}

}  // namespace smirkly::chat::api::health
