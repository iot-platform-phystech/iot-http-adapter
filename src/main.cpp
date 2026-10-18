#include <drogon/drogon.h>

#include <functional>
#include <memory>
#include <string>

int main()
{
    auto &app = drogon::app();

    auto coreClient =
        drogon::HttpClient::newHttpClient("http://127.0.0.1:8000");

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

    app.addListener("127.0.0.1", 8080);
    app.run();
}
