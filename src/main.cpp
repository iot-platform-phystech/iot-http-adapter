#include <drogon/drogon.h>
#include "controllers/AdapterController.h"
#include <cstdlib>
#include <iostream>
namespace {
std::string env(const char* name, const char* fallback = "") {
    const auto value = std::getenv(name);
    return value ? value : fallback;
}
}
int main() {
    try {
        const auto key = iot::Decoder().decode(env("IOT_AES_KEY_BASE64"), "base64");
        auto core = std::make_shared<iot::HttpRepository>(env("IOT_CORE_URL", "http://127.0.0.1:8000"));
        const auto sensorUrl = env("IOT_SENSOR_URL");
        auto sensor = sensorUrl.empty() ? nullptr : std::make_shared<iot::HttpRepository>(sensorUrl);
        auto service = std::make_shared<iot::AdapterService>(core, sensor, key,
            env("IOT_SENSOR_ALGORITHM", "aes-256-gcm"), env("IOT_SENSOR_PATH", "/api/v1/commands"));
        iot::AdapterController(service).registerRoutes();
        drogon::app().setClientMaxBodySize(2 * 1024 * 1024)
            .addListener(env("IOT_ADAPTER_HOST", "127.0.0.1"), 8080).run();
    } catch (const std::exception& e) {
        std::cerr << "Adapter startup failed: " << e.what() << '\n';
        return 1;
    }
}
