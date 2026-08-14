#include "alert.hpp"

#include <curl/curl.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace {

struct CurlEasyDeleter {
    void operator()(CURL* handle) const {
        if (handle != nullptr) {
            curl_easy_cleanup(handle);
        }
    }
};

struct CurlSListDeleter {
    void operator()(curl_slist* list) const {
        if (list != nullptr) {
            curl_slist_free_all(list);
        }
    }
};

using CurlEasyPtr = std::unique_ptr<CURL, CurlEasyDeleter>;
using CurlSListPtr = std::unique_ptr<curl_slist, CurlSListDeleter>;

[[nodiscard]] std::string jsonEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (const char ch : input) {
        switch (ch) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += ch;
                break;
        }
    }
    return out;
}

// Discard Discord response body; we only care about HTTP status.
std::size_t discardWrite(char* /*ptr*/, std::size_t size, std::size_t nmemb, void* /*userdata*/) {
    return size * nmemb;
}

}  // namespace

DiscordAlert::DiscordAlert() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::ifstream in(".env");
    if (!in) {
        std::cerr << "DiscordAlert: could not open .env (webhook disabled).\n";
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        constexpr std::string_view prefix = "DISCORD_WEBHOOK_URL=";
        if (line.starts_with(prefix)) {
            webhookUrl_ = line.substr(prefix.size());
            break;
        }
    }

    if (webhookUrl_.empty()) {
        std::cerr << "DiscordAlert: DISCORD_WEBHOOK_URL missing in .env.\n";
    } else {
        std::cout << "DiscordAlert: webhook loaded from .env.\n";
    }
}

bool DiscordAlert::send(const std::string& message) {
    if (webhookUrl_.empty()) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    if (hasSent_ && (now - lastSend_) < kCooldown) {
        return false;
    }

    const std::string body = "{\"content\":\"" + jsonEscape(message) + "\"}";

    CurlEasyPtr curl(curl_easy_init());
    if (!curl) {
        std::cerr << "DiscordAlert: curl_easy_init failed.\n";
        return false;
    }

    CurlSListPtr headers(curl_slist_append(nullptr, "Content-Type: application/json"));
    if (!headers) {
        std::cerr << "DiscordAlert: could not build HTTP headers.\n";
        return false;
    }

    curl_easy_setopt(curl.get(), CURLOPT_URL, webhookUrl_.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, discardWrite);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 10L);

    const CURLcode result = curl_easy_perform(curl.get());
    if (result != CURLE_OK) {
        std::cerr << "DiscordAlert: request failed (" << curl_easy_strerror(result) << ").\n";
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &httpCode);
    // Discord webhooks typically return 204 No Content on success.
    if (httpCode < 200 || httpCode >= 300) {
        std::cerr << "DiscordAlert: unexpected HTTP status " << httpCode << ".\n";
        return false;
    }

    lastSend_ = now;
    hasSent_ = true;
    std::cout << "DiscordAlert: message sent.\n";
    return true;
}
