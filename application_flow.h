#pragma once
#include <tgbot/tgbot.h>
#include <string>
#include <vector>
#include "tariff_manager.h" // Добавлено

struct UserData;
struct ApplicationDataForReport;
enum class UserState;
enum class AdminWorkMode;
enum class ApplicationStatus;

void sendMainMenu(TgBot::Bot& bot, int64_t chat_id);
void sendTariffSelection(TgBot::Bot& bot, int64_t chat_id);
void sendTradePointSelection(TgBot::Bot& bot, int64_t chat_id);
void askForName(TgBot::Bot& bot, int64_t chat_id); // Теперь для ФИО
void askForPhone(TgBot::Bot& bot, int64_t chat_id);
void askForMessenger(TgBot::Bot& bot, int64_t chat_id);
void askForEmail(TgBot::Bot& bot, int64_t chat_id);
void askForCity(TgBot::Bot& bot, int64_t chat_id);
void askForStreet(TgBot::Bot& bot, int64_t chat_id);
void askForHouse(TgBot::Bot& bot, int64_t chat_id);
void askForHouseBody(TgBot::Bot& bot, int64_t chat_id);
void askForApartment(TgBot::Bot& bot, int64_t chat_id);
void sendHelpMenu(TgBot::Bot& bot, int64_t chat_id);
bool handle_main_menu_buttons(TgBot::Bot& bot, TgBot::Message::Ptr message);
void handle_client_message(TgBot::Bot& bot, TgBot::Message::Ptr message);
void handle_client_callback(TgBot::Bot& bot, TgBot::CallbackQuery::Ptr query);
void sendBackButtonKeyboard(TgBot::Bot& bot, int64_t chat_id, const std::string& text);