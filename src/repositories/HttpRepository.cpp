#include "HttpRepository.h"
namespace iot {
void HttpRepository::request(drogon::HttpMethod method, const std::string& path, const std::string& query,
                             const Json::Value& body, Done done) const {
    auto request = body.isNull() ? drogon::HttpRequest::newHttpRequest() : drogon::HttpRequest::newHttpJsonRequest(body);
    request->setMethod(method);
    request->setPath(path + (query.empty() ? "" : "?" + query));
    client_->sendRequest(request, [done = std::move(done)](drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
        if (result != drogon::ReqResult::Ok || !response) {
            done(failure(502, "Upstream unavailable"));
            return;
        }
        done({static_cast<int>(response->statusCode()), std::string(response->body()), response->getHeader("content-type")});
    }, 5.0);
}
}
