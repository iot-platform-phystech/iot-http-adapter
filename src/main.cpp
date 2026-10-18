#include <drogon/drogon.h>

#include <functional>
#include <memory>
#include <string>
#include <cstdlib>

int main()
{
    auto &app = drogon::app();

    const char *coreUrl = std::getenv("IOT_CORE_URL");
if (coreUrl == nullptr)
{
    coreUrl = "http://127.0.0.1:8000";
}

const char *adapterHost = std::getenv("IOT_ADAPTER_HOST");
if (adapterHost == nullptr)
{
    adapterHost = "127.0.0.1";
}

auto coreClient = drogon::HttpClient::newHttpClient(coreUrl);

    app.registerHandler(
        "/api/v1/ingest",
        [coreClient](
            const drogon::HttpRequestPtr &request,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            auto json = request->getJsonObject();

            if (!json)
            {
                Json::Value error;
                error["error"] = "Expected a JSON request body";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            auto coreRequest = drogon::HttpRequest::newHttpJsonRequest(*json);
            coreRequest->setMethod(drogon::Post);
            coreRequest->setPath("/api/v1/ingest");

            coreClient->sendRequest(
                coreRequest,
                [callback = std::move(callback)](
                    drogon::ReqResult result,
                    const drogon::HttpResponsePtr &coreResponse)
                {
                    if (result != drogon::ReqResult::Ok || !coreResponse)
                    {
                        Json::Value error;
                        error["error"] = "iot-core is unavailable";

                        auto response =
                            drogon::HttpResponse::newHttpJsonResponse(error);
                        response->setStatusCode(drogon::k502BadGateway);
                        callback(response);
                        return;
                    }

                    auto response = drogon::HttpResponse::newHttpResponse();
                    response->setStatusCode(coreResponse->getStatusCode());
                    response->setBody(std::string(coreResponse->getBody()));

                    const auto contentType =
                        coreResponse->getHeader("content-type");

                    if (!contentType.empty())
                    {
                        response->addHeader("Content-Type", contentType);
                    }

                    callback(response);
                },
                5.0);
        },
        {drogon::Post});

    app.addListener(adapterHost, 8080);
    app.run();
}
