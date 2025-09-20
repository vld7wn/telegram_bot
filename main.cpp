#include "Bot.h"
#include <fstream>
#include <nlohmann/json.hpp>

#include "utils/crow_all.h"
#include "utils/logger.h"

// Функция для запуска веб-сервера
void runWebServer() {
    crow::SimpleApp app;

    // 1. Маршрут для отдачи главной страницы index.html
    CROW_ROUTE(app, "/")([](const crow::request&, crow::response& res){
        std::ifstream file("../webapp/index.html");
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_header("Content-Type", "text/html");
            res.write(buffer.str());
        } else {
            res.code = 404;
            res.write("Not Found");
        }
        res.end();
    });

    // 2. Маршрут для отдачи данных о тарифах
    CROW_ROUTE(app, "/api/tariffs")([](const crow::request&, crow::response& res){
        std::ifstream file("../json-cfg/tariff_plan.json");
        if (file) {
            nlohmann::json tariffs = nlohmann::json::parse(file);
            res.set_header("Content-Type", "application/json");
            res.write(tariffs.dump());
        } else {
            res.code = 500;
            res.write("Tariff file not found");
        }
        res.end();
    });

    app.port(18080).multithreaded().run();
}

int main() {
    Logger::get().setup("Logger/bot_log.txt");

    // Запускаем веб-сервер в отдельном потоке
    std::thread server_thread(runWebServer);
    server_thread.detach(); // Отсоединяем поток, чтобы он работал в фоне

    std::ifstream config_file("json-cfg/config.json");
    if (!config_file.is_open()) {
        LOG(LogLevel::L_ERROR, "Could not open config file.");
        return 1;
    }
    nlohmann::json config;
    try {
        config_file >> config;
    } catch (const std::exception& e) {
        LOG(LogLevel::L_ERROR, "Failed to parse config.json: " + std::string(e.what()));
        return 1;
    }

    std::string bot_token = config["bot_token"];

    try {
        Bot bot(bot_token);
        bot.run();
    } catch (const std::exception& e) {
        LOG(LogLevel::L_ERROR, "An exception occurred: " + std::string(e.what()));
    }

    return 0;
}
