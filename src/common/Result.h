#pragma once
#include <json/json.h>
#include <functional>
#include <string>
namespace iot {
struct Result {
    int status;
    std::string body;
    std::string contentType = "application/json";
};
using Done = std::function<void(Result)>;
inline Result failure(int status, const std::string& message) {
    Json::Value body;
    body["error"] = message;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    return {status, Json::writeString(writer, body)};
}
}
