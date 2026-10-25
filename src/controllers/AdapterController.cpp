#include "AdapterController.h"
#include <drogon/drogon.h>
namespace iot {
namespace {
using Callback = std::function<void(const drogon::HttpResponsePtr&)>;
Done respond(Callback callback) {
    return [callback = std::move(callback)](Result result) {
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(static_cast<drogon::HttpStatusCode>(result.status));
        response->setBody(std::move(result.body));
        if (!result.contentType.empty()) response->addHeader("content-type", result.contentType);
        callback(response);
    };
}
}
void AdapterController::registerRoutes() const {
    drogon::app().registerHandler("/ping", [](const drogon::HttpRequestPtr&, Callback&& callback) {
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setContentTypeCode(drogon::CT_TEXT_PLAIN);
        response->setBody("pong");
        callback(response);
    }, {drogon::Get});
    for (const auto& path : {"/api/v1/ingest", "/api/v1/commands"}) {
        drogon::app().registerHandler(path, [service = service_](const drogon::HttpRequestPtr& request, Callback&& callback) {
            auto done = respond(std::move(callback));
            auto body = request->getJsonObject();
            if (!body) { done(failure(400, "Expected JSON body")); return; }
            if (request->path() == "/api/v1/ingest") service->ingest(*body, std::move(done));
            else service->command(*body, std::move(done));
        }, {drogon::Post});
    }
    auto proxy = [service = service_](const drogon::HttpRequestPtr& request, Callback&& callback) {
        auto done = respond(std::move(callback));
        Json::Value body;
        if (request->method() == drogon::Post || request->method() == drogon::Put) {
            auto parsed = request->getJsonObject();
            if (!parsed || !parsed->isObject()) { done(failure(400, "Expected JSON object")); return; }
            body = *parsed;
        }
        service->proxy(request->method(), request->path(), request->query(), body, std::move(done));
    };
    for (const std::string resource : {"users", "devices"}) {
        const auto path = "/api/v1/" + resource;
        drogon::app().registerHandler(path, proxy, {drogon::Get, drogon::Post});
        drogon::app().registerHandler(path + "/{1}",
            [proxy](const drogon::HttpRequestPtr& request, Callback&& callback, std::string) {
                proxy(request, std::move(callback));
            }, {drogon::Get, drogon::Put, drogon::Delete});
    }
    drogon::app().registerHandler("/api/v1/data", proxy, {drogon::Get});
    drogon::app().registerHandler("/api/v1/devices/{1}/commands",
        [proxy](const drogon::HttpRequestPtr& request, Callback&& callback, std::string) {
            proxy(request, std::move(callback));
        }, {drogon::Post});
}
}
