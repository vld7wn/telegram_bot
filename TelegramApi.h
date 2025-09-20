#pragma once
#include <string>
#include <nlohmann/json.hpp>

class TelegramApi {
public:
    explicit TelegramApi(const std::string& token);

    void sendMessage(long long chat_id, const std::string& text, const nlohmann::json& reply_markup = nullptr);
    void answerCallbackQuery(const std::string& callback_query_id);
    nlohmann::json getUpdates(long long offset);

private:
    std::string base_url;
};