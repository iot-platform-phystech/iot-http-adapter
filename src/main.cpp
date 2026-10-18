#include <drogon/drogon.h>

void pingHandler(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(drogon::k200OK);
    response->setContentTypeCode(drogon::CT_TEXT_PLAIN);
    response->setBody("pong");

    callback(response);
}

int main()
{
    drogon::app().registerHandler(
        "/ping",
        pingHandler,
        {drogon::Get}
    );

    drogon::app()
        .addListener("127.0.0.1", 8080)
        .run();
}