#include "TelegramApi.h"
#include <cpr/cpr.h>

#include "utils/logger.h"

TelegramApi::TelegramApi(const std::string& token) {
    base_url = "https://api.telegram.org/bot" + token;
}

void TelegramApi::sendMessage(long long chat_id, const std::string& text, const nlohmann::json& reply_markup) {
    nlohmann::json payload = {
        {"chat_id", chat_id},
        {"text", text}
    };
    if (reply_markup != nullptr) {
        payload["reply_markup"] = reply_markup;
    }

    cpr::Response r = cpr::Post(cpr::Url{base_url + "/sendMessage"},
                                cpr::Header{{"Content-Type", "application/json"}},
                                cpr::Body{payload.dump()},
                                cpr::Timeout{10000}); // Timeout 10 seconds
    if (r.status_code != 200) {
        LOG(LogLevel::L_ERROR, "Failed to send message: " + r.text);
    }
}

void TelegramApi::answerCallbackQuery(const std::string& callback_query_id) {
    nlohmann::json payload = {{"callback_query_id", callback_query_id}};
    cpr::Post(cpr::Url{base_url + "/answerCallbackQuery"},
              cpr::Header{{"Content-Type", "application/json"}},
              cpr::Body{payload.dump()},
              cpr::Timeout{5000}); // Timeout 5 seconds
}

nlohmann::json TelegramApi::getUpdates(long long offset) {
    std::string updates_url = base_url + "/getUpdates?offset=" + std::to_string(offset) + "&timeout=10";

    cpr::Response r = cpr::Get(cpr::Url{updates_url}, cpr::Timeout{15000}); // Timeout 15 seconds

    if (r.status_code != 200) {
        LOG(LogLevel::L_ERROR, "Failed to get updates. Status: " + std::to_string(r.status_code) + ", Error: " + r.error.message);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return nullptr;
    }
    if (r.text.empty()) {
        LOG(LogLevel::L_WARNING, "Received empty response from getUpdates.");
        return nullptr;
    }
    return nlohmann::json::parse(r.text, nullptr, false); // Safe parsing
}