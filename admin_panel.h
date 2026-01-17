#pragma once
#include <tgbot/tgbot.h>
#include <string>
#include <vector>
#include "user_data_types.h"
#include "application_status.h"
#include "message_to_client.h"

// Функции-обработчики сообщений, разделенные для читаемости
void handle_admin_name_input_message(TgBot::Bot& bot, TgBot::Message::Ptr message);
void handle_admin_otp_input_message(TgBot::Bot& bot, TgBot::Message::Ptr message);
void handle_admin_buttons_message(TgBot::Bot& bot, TgBot::Message::Ptr message);

// Главный обработчик сообщений для админ-панели
void handle_admin_panel_message(TgBot::Bot& bot, TgBot::Message::Ptr message);

// Вспомогательные функции для админ-панели
void sendAdminPanel(TgBot::Bot& bot, int64_t chat_id);
void sendApplicationsForReview(TgBot::Bot& bot, int64_t chat_id, const std::string& trade_point);
void start_admin_registration(TgBot::Bot& bot, int64_t chat_id);
void handle_admin_callbacks(TgBot::Bot& bot, TgBot::CallbackQuery::Ptr query);

// Функции, связанные с регистрацией и авторизацией админов
void send_approval_request(TgBot::Bot& bot, int64_t new_admin_id, const std::string& name, const std::string& trade_point);
void send_otp(TgBot::Bot& bot, int64_t admin_id, const std::string& reason);
void notifyAdminsOfNewApplication(TgBot::Bot& bot, const std::string& trade_point, const std::string& message);
