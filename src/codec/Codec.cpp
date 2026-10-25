#include "Codec.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <climits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace iot {
namespace {
constexpr std::size_t maxPayload = 1024 * 1024;
void checkSize(std::string_view data) {
    if (data.size() > maxPayload) throw std::invalid_argument("Payload too large");
}
void checkKey(const std::string& key) {
    if (!key.empty() && key.size() != 32)
        throw std::invalid_argument("AES key must contain exactly 32 bytes");
}
void requireKey(const std::string& key) {
    if (key.size() != 32) throw std::invalid_argument("AES key is not configured");
}
void check(int result) {
    if (result != 1) throw std::runtime_error("Cryptographic operation failed");
}
std::string base64(std::string_view data) {
    std::string result(4 * ((data.size() + 2) / 3) + 1, '\0');
    const int length = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(result.data()),
        reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size()));
    result.resize(length);
    return result;
}
std::string unbase64(std::string_view data) {
    if (data.empty()) return {};
    if (data.size() % 4 != 0) throw std::invalid_argument("Invalid Base64");
    std::size_t padding = data.back() == '=' ? 1 : 0;
    if (data[data.size() - 2] == '=') ++padding;
    for (std::size_t i = 0; i < data.size() - padding; ++i) {
        const char c = data[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '+' || c == '/'))
            throw std::invalid_argument("Invalid Base64");
    }
    std::string result(data.size() / 4 * 3, '\0');
    int length = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(result.data()),
        reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size()));
    if (length < 0) throw std::invalid_argument("Invalid Base64");
    result.resize(length - padding);
    if (base64(result) != data) throw std::invalid_argument("Non-canonical Base64");
    return result;
}
using Context = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;
Context context() {
    Context ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) throw std::runtime_error("Cannot allocate cipher context");
    return ctx;
}
}
Encoder::Encoder(std::string key) : key_(std::move(key)) { checkKey(key_); }
Decoder::Decoder(std::string key) : key_(std::move(key)) { checkKey(key_); }

std::string Encoder::encode(std::string_view data, std::string_view algorithm) const {
    checkSize(data);
    if (algorithm == "base64") return base64(data);
    if (algorithm != "aes-256-gcm") throw std::invalid_argument("Unsupported algorithm");
    requireKey(key_);
    unsigned char nonce[12];
    check(RAND_bytes(nonce, sizeof(nonce)));
    auto ctx = context();
    check(EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr,
        reinterpret_cast<const unsigned char*>(key_.data()), nonce));
    std::string encrypted(data.size() + 16, '\0');
    int length = 0, tail = 0;
    check(EVP_EncryptUpdate(ctx.get(), reinterpret_cast<unsigned char*>(encrypted.data()),
        &length, reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size())));
    check(EVP_EncryptFinal_ex(ctx.get(), reinterpret_cast<unsigned char*>(encrypted.data()) + length, &tail));
    encrypted.resize(length + tail);
    unsigned char tag[16];
    check(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, sizeof(tag), tag));
    return base64(std::string(reinterpret_cast<char*>(nonce), sizeof(nonce)) + encrypted +
        std::string(reinterpret_cast<char*>(tag), sizeof(tag)));
}

std::string Decoder::decode(std::string_view data, std::string_view algorithm) const {
    if (data.size() > 4 * ((maxPayload + 28 + 2) / 3))
        throw std::invalid_argument("Payload too large");
    if (algorithm == "base64") {
        auto decoded = unbase64(data);
        checkSize(decoded);
        return decoded;
    }
    if (algorithm != "aes-256-gcm") throw std::invalid_argument("Unsupported algorithm");
    requireKey(key_);
    auto packed = unbase64(data);
    if (packed.size() < 28) throw std::invalid_argument("Invalid AES payload");
    if (packed.size() - 28 > maxPayload) throw std::invalid_argument("Payload too large");
    auto ctx = context();
    auto bytes = reinterpret_cast<unsigned char*>(packed.data());
    check(EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr,
        reinterpret_cast<const unsigned char*>(key_.data()), bytes));
    const int ciphertextLength = static_cast<int>(packed.size() - 28);
    std::string plaintext(ciphertextLength + 16, '\0');
    int length = 0, tail = 0;
    check(EVP_DecryptUpdate(ctx.get(), reinterpret_cast<unsigned char*>(plaintext.data()),
        &length, bytes + 12, ciphertextLength));
    check(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16, bytes + packed.size() - 16));
    if (EVP_DecryptFinal_ex(ctx.get(), reinterpret_cast<unsigned char*>(plaintext.data()) + length, &tail) != 1)
        throw std::invalid_argument("AES authentication failed");
    plaintext.resize(length + tail);
    return plaintext;
}
}  // namespace iot
