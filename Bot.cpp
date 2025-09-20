#include "Bot.h"
#include "Buttons.h"
#include <iostream>
#include <thread>

#include "utils/logger.h"

Bot::Bot(const std::string& token) : api(token) {
    LOG(LogLevel::INFO, "Bot initialized.");
}

void Bot::run() {
    LOG(LogLevel::INFO, "Bot is running...");
    while (true) {
        try {
            auto json_response = api.getUpdates(offset);

            if (json_response == nullptr || json_response.is_discarded() || json_response["ok"] != true) {
                continue;
            }

            for (const auto& update : json_response["result"]) {
                process_update(update);
            }
        } catch (const std::exception& e) {
            LOG(LogLevel::L_ERROR, "Exception in main loop: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void Bot::process_update(const nlohmann::json& update) {
    offset = update["update_id"].get<long long>() + 1;

    if (update.contains("message")) {
        process_message(update["message"]);
    } else if (update.contains("callback_query")) {
        process_callback_query(update["callback_query"]);
    }
}

void Bot::process_message(const nlohmann::json& message) {
    long long chat_id = message["chat"]["id"];

    // --- ОБРАБОТКА ДАННЫХ ИЗ ВЕБ-ФОРМЫ ---
    if (message.contains("web_app_data")) {
        std::string web_app_data_str = message["web_app_data"]["data"];
        LOG(LogLevel::INFO, "Received Web App data from chat_id=" + std::to_string(chat_id) + ": " + web_app_data_str);

        try {
            auto data = nlohmann::json::parse(web_app_data_str);
            // Выводим в лог все новые данные для проверки
            LOG(LogLevel::INFO, "Parsed FIO: " + data.value("fio", "N/A"));
            LOG(LogLevel::INFO, "Parsed Phone: " + data.value("phone", "N/A"));
            LOG(LogLevel::INFO, "Parsed Email: " + data.value("email", "N/A"));
            LOG(LogLevel::INFO, "Parsed Address: " + data.value("address", "N/A"));
            LOG(LogLevel::INFO, "Parsed App Type: " + data.value("application_type", "N/A"));

            api.sendMessage(chat_id, "Спасибо, ваша заявка обновлена и принята в обработку!");

        } catch (const std::exception& e) {
            LOG(LogLevel::L_ERROR, "Failed to parse Web App data: " + std::string(e.what()));
            api.sendMessage(chat_id, "Произошла ошибка при обработке ваших данных.");
        }
        return;
    }

    // --- ОБРАБОТКА ТЕКСТОВЫХ КОМАНД ---
    if (!message.contains("text")) return;
    std::string text = message["text"];
    LOG(LogLevel::INFO, "Received message from chat_id=" + std::to_string(chat_id) + ": '" + text + "'");

    if (text == "/start") {
        handle_start_command(chat_id);
    }
}

void Bot::process_callback_query(const nlohmann::json& query) {
    long long chat_id = query["message"]["chat"]["id"];
    std::string callback_data = query["data"];
    api.answerCallbackQuery(query["id"]);
    LOG(LogLevel::INFO, "Received callback from chat_id=" + std::to_string(chat_id) + ": '" + callback_data + "'");


    if (callback_data == "instruction_seen") {
        std::string menu_text = "Отлично! Теперь вы можете оставить заявку или проверить статус существующей.";
        api.sendMessage(chat_id, menu_text, Buttons::get_main_menu_keyboard());
    }
    else if (callback_data == "check_request") {
        api.sendMessage(chat_id, "Функция проверки статуса заявки находится в разработке.");
    }
}

void Bot::handle_start_command(long long chat_id) {
    std::string welcome_text =
        "Здравствуйте!\n\n"
        "Это бот для подачи заявок на подключение домашнего интернета МТС.\n\n"
        "**Как это работает:**\n"
        "1. Нажмите 'Оставить заявку', чтобы открыть форму.\n"
        "2. Заполните все поля и подтвердите отправку.\n\n"
        "Пожалуйста, подтвердите, что вы ознакомились с инструкцией.";

    api.sendMessage(chat_id, welcome_text, Buttons::get_start_keyboard());
    LOG(LogLevel::INFO, "Sent welcome message to chat_id=" + std::to_string(chat_id));
}