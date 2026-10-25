#pragma once
#include "codec/Codec.h"
#include "repositories/HttpRepository.h"
namespace iot {
class AdapterService {
public:
    AdapterService(std::shared_ptr<HttpRepository> core, std::shared_ptr<HttpRepository> sensor,
                   std::string key, std::string algorithm, std::string sensorPath);
    void ingest(const Json::Value&, Done) const;
    void command(const Json::Value&, Done) const;
    void proxy(drogon::HttpMethod method, const std::string& path, const std::string& query,
               const Json::Value& body, Done done) const;
private:
    std::shared_ptr<HttpRepository> core_, sensor_;
    Encoder encoder_;
    Decoder decoder_;
    std::string algorithm_, sensorPath_;
};
}
