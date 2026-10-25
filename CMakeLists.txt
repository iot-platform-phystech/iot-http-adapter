cmake_minimum_required(VERSION 3.24)

project(iot_http_adapter
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(iot-codec src/codec/Codec.cpp)
target_include_directories(iot-codec PUBLIC src)
target_link_libraries(iot-codec PUBLIC OpenSSL::Crypto)

add_executable(iot-http-adapter
    src/main.cpp
    src/controllers/AdapterController.cpp
    src/services/AdapterService.cpp
    src/repositories/HttpRepository.cpp
)

target_link_libraries(iot-http-adapter
    PRIVATE
        Drogon::Drogon
        iot-codec
)

include(CTest)
if(BUILD_TESTING)
    add_executable(codec-tests tests/CodecTest.cpp)
    target_link_libraries(codec-tests PRIVATE iot-codec)
    add_test(NAME codec-tests COMMAND codec-tests)
endif()
