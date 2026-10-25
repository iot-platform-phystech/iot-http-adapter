#pragma once

#include <string>
#include <string_view>

namespace iot {
// AES payload: Base64(12-byte nonce || ciphertext || 16-byte authentication tag).
// The key is exactly 32 raw bytes; an empty key allows Base64 only.
class Encoder {
public:
    explicit Encoder(std::string key = {});
    std::string encode(std::string_view data, std::string_view algorithm) const;
private:
    std::string key_;
};

class Decoder {
public:
    explicit Decoder(std::string key = {});
    std::string decode(std::string_view data, std::string_view algorithm) const;
private:
    std::string key_;
};
}  // namespace iot
