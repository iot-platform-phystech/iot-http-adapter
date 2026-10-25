#include "codec/Codec.h"
#include <iostream>
#include <stdexcept>

void require(bool ok) { if (!ok) throw std::runtime_error("Test failed"); }
template<class F> void rejects(F f) {
    try { f(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("Expected rejection");
}
int main() {
    try {
        iot::Encoder encoder(std::string(32, '\0'));
        iot::Decoder decoder(std::string(32, '\0'));
        require(encoder.encode("foobar", "base64") == "Zm9vYmFy");
        require(decoder.decode("Zg==", "base64") == "f");
        for (const auto& value : {std::string(), std::string("hello"), std::string("\0\xff\0", 3), std::string(1024 * 1024, 'x')}) {
            for (const auto algorithm : {"base64", "aes-256-gcm"})
                require(decoder.decode(encoder.encode(value, algorithm), algorithm) == value);
        }
        for (const auto bad : {"Zg=", "Zg===", "!!!!", "Zh==", "Zm9=", "=AAA", "Z g="})
            rejects([&] { decoder.decode(bad, "base64"); });
        rejects([&] { encoder.encode("x", "unknown"); });
        rejects([&] { decoder.decode("", "unknown"); });
        rejects([] { iot::Encoder("short"); });
        rejects([] { iot::Encoder().encode("x", "aes-256-gcm"); });
        rejects([&] { decoder.decode("", "aes-256-gcm"); });
        auto encrypted = encoder.encode("secret", "aes-256-gcm");
        require(encrypted != encoder.encode("secret", "aes-256-gcm"));
        rejects([&] { iot::Decoder(std::string(32, 'x')).decode(encrypted, "aes-256-gcm"); });
        auto packed = decoder.decode(encrypted, "base64");
        for (const std::size_t position : {std::size_t(0), std::size_t(12), packed.size() - 1}) {
            auto altered = packed;
            altered[position] ^= 1;
            rejects([&] { decoder.decode(encoder.encode(altered, "base64"), "aes-256-gcm"); });
        }
        // NIST AES-256-GCM empty-plaintext vector, zero key and zero nonce.
        const unsigned char tag[] = {0x53,0x0f,0x8a,0xfb,0xc7,0x45,0x36,0xb9,0xa9,0x63,0xb4,0xf1,0xc4,0xcb,0x73,0x8b};
        auto vector = std::string(12, '\0') + std::string(reinterpret_cast<const char*>(tag), 16);
        require(decoder.decode(encoder.encode(vector, "base64"), "aes-256-gcm").empty());
        rejects([&] { encoder.encode(std::string(1024 * 1024 + 1, 'x'), "base64"); });
        std::cout << "Codec tests passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}
