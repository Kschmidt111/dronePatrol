#pragma once

#include <chrono>
#include <string>

class DiscordAlert {
public:
    explicit DiscordAlert();

    // Posts message to the webhook. Honors cooldown between successful sends.
    [[nodiscard]] bool send(const std::string& message);

    [[nodiscard]] bool ready() const { return !webhookUrl_.empty(); }

private:
    std::string webhookUrl_;
    std::chrono::steady_clock::time_point lastSend_{};
    bool hasSent_ = false;

    static constexpr std::chrono::seconds kCooldown{30};
};
