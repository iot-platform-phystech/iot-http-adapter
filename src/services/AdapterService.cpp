#include "AdapterService.h"
#include <cmath>
#include <stdexcept>
namespace iot {
AdapterService::AdapterService(std::shared_ptr<HttpRepository> core, std::shared_ptr<HttpRepository> sensor,
                               std::string key, std::string algorithm, std::string sensorPath)
    : core_(std::move(core)), sensor_(std::move(sensor)), encoder_(key), decoder_(key),
      algorithm_(std::move(algorithm)), sensorPath_(std::move(sensorPath)) {
    encoder_.encode("", algorithm_);
}
void AdapterService::ingest(const Json::Value& envelope, Done done) const {
    Json::Value decoded;
    try {
        if (!envelope.isObject() || !envelope["algorithm"].isString() || !envelope["payload"].isString())
            throw std::invalid_argument("Expected algorithm and payload strings");
        if (envelope["algorithm"].asString() != algorithm_)
            throw std::invalid_argument("Algorithm does not match adapter configuration");
        const auto plaintext = decoder_.decode(envelope["payload"].asString(), algorithm_);
        Json::CharReaderBuilder builder;
        builder["failIfExtra"] = true;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errors;
        if (!reader->parse(plaintext.data(), plaintext.data() + plaintext.size(), &decoded, &errors) ||
            !decoded.isObject() || !decoded["sensor_id"].isString() || decoded["sensor_id"].asString().empty() ||
            !decoded["value"].isNumeric() || !std::isfinite(decoded["value"].asDouble()) ||
            (decoded.isMember("unit") && !decoded["unit"].isString()))
            throw std::invalid_argument("Invalid sensor JSON: expected sensor_id, numeric value, optional unit");
    } catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
    catch (const std::exception&) { done(failure(500, "Decoding failed")); return; }
    core_->request(drogon::Post, "/api/v1/ingest", "", decoded, std::move(done));
}
void AdapterService::command(const Json::Value& command, Done done) const {
    if (!sensor_) { done(failure(503, "IOT_SENSOR_URL is not configured")); return; }
    if (!command.isObject()) { done(failure(400, "Expected command JSON object")); return; }
    Json::Value envelope;
    try {
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        envelope["algorithm"] = algorithm_;
        envelope["payload"] = encoder_.encode(Json::writeString(writer, command), algorithm_);
    } catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
    catch (const std::exception&) { done(failure(500, "Encoding failed")); return; }
    sensor_->request(drogon::Post, sensorPath_, "", envelope, std::move(done));
}
void AdapterService::proxy(drogon::HttpMethod method, const std::string& path, const std::string& query,
                           const Json::Value& body, Done done) const {
    core_->request(method, path, query, body, std::move(done));
}
}
