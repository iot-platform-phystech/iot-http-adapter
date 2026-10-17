cmake_minimum_required(VERSION 3.24)

project(iot_http_adapter
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)

add_executable(iot-http-adapter
    src/main.cpp
)

target_link_libraries(iot-http-adapter
    PRIVATE
        Drogon::Drogon
)