#pragma once
#include "TelegramApi.h"

class Bot {
public:
    explicit Bot(const std::string& token);
    void run();

private:
    void process_update(const nlohmann::json& update);
    void process_message(const nlohmann::json& message);
    void process_callback_query(const nlohmann::json& query);

    void handle_start_command(long long chat_id);

    TelegramApi api;
    long long offset = 0;
};