#pragma once
#include "common/Result.h"
#include <drogon/HttpClient.h>
namespace iot {
class HttpRepository {
public:
    explicit HttpRepository(const std::string& origin) : client_(drogon::HttpClient::newHttpClient(origin)) {}
    void request(drogon::HttpMethod method, const std::string& path, const std::string& query,
                 const Json::Value& body, Done done) const;
private:
    drogon::HttpClientPtr client_;
};
}
