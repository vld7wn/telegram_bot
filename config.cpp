#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include "logger.h"

Config config;

void load_config(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open())
    {
        throw std::runtime_error("Could not open config file: " + path);
    }

    try
    {
        nlohmann::json data = nlohmann::json::parse(f);
        config.bot_token = data.at("bot_token").get<std::string>();
        config.main_admin_id = data.at("main_admin_id").get<int64_t>();

        // Опциональные поля логирования (используем значения по умолчанию если нет в JSON)
        if (data.contains("log_level"))
        {
            config.log_level = data["log_level"].get<std::string>();
        }
        if (data.contains("log_file"))
        {
            config.log_file = data["log_file"].get<std::string>();
        }
        if (data.contains("webapp_url"))
        {
            config.webapp_url = data["webapp_url"].get<std::string>();
        }
    }
    catch (const nlohmann::json::parse_error &e)
    {
        LOG(LogLevel::L_ERROR, "JSON parse error in " << path << ": " << e.what());
        throw std::runtime_error("Invalid JSON syntax in " + path);
    }
    catch (const nlohmann::json::out_of_range &e)
    {
        LOG(LogLevel::L_ERROR, "Missing required field in " << path << ": " << e.what());
        throw std::runtime_error("Missing required field in " + path);
    }
}