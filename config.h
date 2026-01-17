#pragma once
#include <string>
#include <cstdint>

struct Config
{
    std::string bot_token;
    int64_t main_admin_id;
    std::string log_level = "INFO";        // INFO, WARNING, ERROR
    std::string log_file = "logs/bot.log"; // Путь к файлу логов
    std::string webapp_url = "";           // URL Telegram WebApp (пустой = использовать inline)
};

extern Config config;
void load_config(const std::string &path = "config.json");