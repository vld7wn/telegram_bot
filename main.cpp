#include <iostream>
#include <tgbot/tgbot.h>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <atomic>
#include <csignal>
#include <nlohmann/json.hpp>
#include "main.h"
#include "config.h"
#include "database.h"
#include "trade_points.h"
#include "application_flow.h"
#include "admin_panel.h"
#include "super_admin.h"
#include "faq_manager.h"
#include "logger.h"
#include "tariff_manager.h"
#include <filesystem>
#include "user_data_types.h"
#include "application_status.h"
#include "message_to_client.h"
#include "http_server.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

// ================== GRACEFUL SHUTDOWN ==================
std::atomic<bool> bot_running{true};

#if defined(_WIN32) || defined(_WIN64)
BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT)
    {
        LOG(LogLevel::INFO, "Received shutdown signal, stopping bot...");
        bot_running = false;
        return TRUE;
    }
    return FALSE;
}
#else
void signalHandler(int signum)
{
    LOG(LogLevel::INFO, "Received signal " << signum << ", stopping bot...");
    bot_running = false;
}
#endif

// ================== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ (теперь через SessionManager) ==================
// Для обратной совместимости создаём ссылки на данные в SessionManager
#include "session_manager.h"

// Эти переменные остаются для совместимости с существующим кодом,
// но фактические данные хранятся в SessionManager
std::map<int64_t, std::string> &admin_otps = []() -> std::map<int64_t, std::string> &
{
    static std::map<int64_t, std::string> otps;
    return otps;
}();
std::map<int64_t, UserData> &user_session_data = SessionManager::instance().getAllUserSessions();
std::map<int64_t, AdminWorkMode> &admin_work_mode = SessionManager::instance().getAllAdminModes();

// ================== ОСНОВНАЯ ЛОГИКА БОТА ==================
int main()
{
// Настройка обработчиков сигналов для graceful shutdown
#if defined(_WIN32) || defined(_WIN64)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#else
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
#endif

    std::cerr << "Starting bot initialization...\n";

    try
    {
        std::cerr << "Attempting to create directory 'db'...\n";
        std::filesystem::create_directory("db");
        std::cerr << "Directory 'db' checked/created.\n";

        std::cerr << "Attempting to create directory 'logs'...\n";
        std::filesystem::create_directory("logs");
        std::cerr << "Directory 'logs' checked/created.\n";

        // Сначала загружаем конфигурацию (без логирования, т.к. logger ещё не инициализирован)
        std::cerr << "Loading configuration...\n";
        load_config("json-cfg/config.json");
        std::cerr << "Config loaded.\n";

        // Теперь настраиваем logger из конфигурации
        std::cerr << "Setting up logger...\n";
        Logger::get().setup(config.log_file);
        Logger::get().setMinLevel(config.log_level);
        std::cerr << "Logger setup completed.\n";

        LOG(LogLevel::INFO, "Configuration loading sequence started.");
        LOG(LogLevel::INFO, "Config loaded from json-cfg/config.json");
        LOG(LogLevel::INFO, "Log level: " << config.log_level << ", Log file: " << config.log_file);
        load_trade_points("json-cfg/trade_points.json");
        LOG(LogLevel::INFO, "Trade points loaded.");
        load_faq("json-cfg/faq.json");
        LOG(LogLevel::INFO, "FAQ loaded.");
        load_tariff_plans("json-cfg/tariff_plann.json"); // <-- ИСПРАВЛЕНО ИМЯ ФАЙЛА
        LOG(LogLevel::INFO, "Tariff plans loaded.");
    }
    catch (const std::exception &e)
    {
        std::cerr << "FATAL ERROR (Caught early): " << e.what() << "\n";
        LOG(LogLevel::L_ERROR, "FATAL ERROR during setup: " << e.what());
        return 1;
    }

    LOG(LogLevel::INFO, "Configuration loaded successfully. Starting bot...");

    db_init();
    LOG(LogLevel::INFO, "Database initialized.");
    db_load_user_states(user_session_data);
    LOG(LogLevel::INFO, "User states loaded.");
    db_load_admin_work_modes(admin_work_mode);
    LOG(LogLevel::INFO, "Admin work modes loaded.");

    TgBot::Bot bot(std::string(config.bot_token));

    // Initialize and start HTTP API server
    initHttpServer(bot);
    HttpServer *httpServer = getHttpServer();
    httpServer->start(8080);
    LOG(LogLevel::INFO, "HTTP API server started on port 8080");

    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message)
                              {
                                  int64_t chat_id = message->chat->id;
                                  LOG(LogLevel::INFO, "Received /start command from chat ID: " << chat_id);
                                  if (chat_id == config.main_admin_id)
                                  {
                                      admin_work_mode[chat_id] = AdminWorkMode::ADMIN_VIEW;
                                      db_save_admin_work_mode(chat_id, AdminWorkMode::ADMIN_VIEW);
                                      sendSuperAdminPanel(bot, chat_id);
                                  }
                                  else
                                  {
                                      sendMainMenu(bot, chat_id);
                                  }
                              });

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message)
                                 {
                                     int64_t chat_id = message->chat->id;
                                     std::string text = message->text;
                                     LOG(LogLevel::INFO, "Received message from chat ID: " << chat_id << ", text: " << text);

                                     // --- СПЕЦИАЛЬНАЯ ОБРАБОТКА ДЛЯ ГЛАВНОГО АДМИНА ---
                                     if (chat_id == config.main_admin_id)
                                     {
                                         AdminWorkMode current_mode = AdminWorkMode::UNKNOWN;
                                         if (admin_work_mode.count(chat_id))
                                         {
                                             current_mode = admin_work_mode[chat_id];
                                         }
                                         else
                                         {
                                             current_mode = db_get_admin_work_mode(chat_id);
                                             admin_work_mode[chat_id] = current_mode;
                                         }
                                         LOG(LogLevel::INFO, "Message from main admin (ID: " << chat_id << "), detected work mode: " << static_cast<int>(current_mode));

                                         UserData &user = user_session_data[chat_id];
                                         if (user.state == UserState::ADMIN_REPLYING_TO_USER || user.state == UserState::AWAITING_ADMIN_PASSWORD)
                                         {
                                             handle_admin_panel_message(bot, message);
                                             return;
                                         }

                                         handle_super_admin_message(bot, message);
                                         return;
                                     }

                                     // --- ОБРАБОТКА СООБЩЕНИЙ ОБЫЧНЫХ ПОЛЬЗОВАТЕЛЕЙ И ОБЫЧНЫХ АДМИНОВ ---
                                     if (!db_get_bot_status())
                                     {
                                         bot.getApi().sendMessage(chat_id, "Бот временно недоступен для обработки запросов. Пожалуйста, попробуйте позже.");
                                         LOG(LogLevel::INFO, "Rejected message from user " << chat_id << " because bot is inactive.");
                                         return;
                                     }

                                     UserData &user = user_session_data[chat_id];

                                     // --- ОБРАБОТКА ДАННЫХ ИЗ WEBAPP ---
                                     if (message->webAppData != nullptr)
                                     {
                                         LOG(LogLevel::INFO, "Received WebApp data from chat ID: " << chat_id);
                                         try
                                         {
                                             nlohmann::json data = nlohmann::json::parse(message->webAppData->data);

                                             // Заполняем UserData из JSON
                                             user.flyer_code = data.value("tradePoint", "");
                                             user.final_tariff_string = data.value("tariff", "") + " (" + data.value("speed", "") + ")";
                                             user.name = data.value("name", "");
                                             user.phone = data.value("phone", "");
                                             user.email = data.value("email", "");
                                             user.city = data.value("city", "");
                                             user.street = data.value("street", "");
                                             user.house = data.value("house", "");
                                             user.apartment = data.value("apartment", "не указана");
                                             user.needs_tv_box = data.value("needsTvBox", false);

                                             // Получаем адрес офиса по коду торговой точки
                                             std::string office_address;
                                             get_address_by_code(user.flyer_code, office_address);

                                             // Расчёт стоимости с учётом аренды роутера и ТВ-приставки
                                             int tariff_price = 0;
                                             int router_rental = 149; // Фиксированная аренда роутера
                                             int tv_box_rental = 0;
                                             try
                                             {
                                                 tariff_price = std::stoi(data.value("price", "0"));
                                             }
                                             catch (...)
                                             {
                                             }

                                             if (user.needs_tv_box)
                                             {
                                                 tv_box_rental = 149; // Аренда ТВ-приставки
                                             }

                                             int total_monthly = tariff_price + router_rental + tv_box_rental;

                                             std::string full_address = "г. " + user.city + ", ул. " + user.street + ", д. " + user.house;
                                             if (!user.apartment.empty() && user.apartment != "не указана")
                                             {
                                                 full_address += ", кв. " + user.apartment;
                                             }

                                             db_add_application(chat_id, user, full_address, total_monthly);

                                             std::stringstream confirmation;
                                             confirmation << "✅ *Ваша заявка принята!*\n\n"
                                                          << "Скоро с вами свяжутся для уточнения деталей.\n\n";
                                             if (!office_address.empty())
                                             {
                                                 confirmation << "Для подключения интернета подойдите по адресу:\n*"
                                                              << office_address << "*\n\n"
                                                              << "**Не забудьте взять с собой паспорт!**";
                                             }

                                             bot.getApi().sendMessage(chat_id, confirmation.str(), false, 0, nullptr, "Markdown");

                                             LOG(LogLevel::INFO, "WebApp application saved for user " << chat_id << ", total: " << total_monthly);
                                         }
                                         catch (const std::exception &e)
                                         {
                                             LOG(LogLevel::L_ERROR, "Failed to parse WebApp data: " << e.what());
                                             bot.getApi().sendMessage(chat_id, "Ошибка обработки заявки. Попробуйте ещё раз.");
                                         }
                                         sendPostApplicationMenu(bot, chat_id);
                                         return;
                                     }

                                     if (handle_main_menu_buttons(bot, message))
                                     {
                                         return;
                                     }

                                     std::string admin_trade_point;
                                     if (db_is_admin_approved(chat_id, admin_trade_point))
                                     {
                                         if (user.state == UserState::ADMIN_REPLYING_TO_USER || user.state == UserState::AWAITING_ADMIN_PASSWORD)
                                         {
                                             handle_admin_panel_message(bot, message);
                                             return;
                                         }
                                     }

                                     switch (user.state)
                                     {
                                     case UserState::AWAITING_ADMIN_NAME:
                                     case UserState::AWAITING_ADMIN_PASSWORD:
                                     case UserState::ADMIN_PANEL:
                                     case UserState::ADMIN_REPLYING_TO_USER:
                                         handle_admin_panel_message(bot, message);
                                         break;
                                     default:
                                         handle_client_message(bot, message);
                                         break;
                                     }
                                 });

    bot.getEvents().onCallbackQuery([&bot](TgBot::CallbackQuery::Ptr query)
                                    {
                                        int64_t chat_id = query->message->chat->id;
                                        LOG(LogLevel::INFO, "Received callback query from chat ID: " << chat_id << ", data: " << query->data);

                                        AdminWorkMode current_mode = AdminWorkMode::UNKNOWN;
                                        if (admin_work_mode.count(chat_id))
                                        {
                                            current_mode = admin_work_mode[chat_id];
                                        }
                                        else
                                        {
                                            current_mode = db_get_admin_work_mode(chat_id);
                                            admin_work_mode[chat_id] = current_mode;
                                        }
                                        LOG(LogLevel::INFO, "Callback query from chat ID: " << chat_id << ", detected work mode: " << static_cast<int>(current_mode));

                                        if (chat_id == config.main_admin_id)
                                        {
                                            handle_super_admin_callbacks(bot, query);
                                            if (query->data.rfind("admin_tp_", 0) == 0 || query->data.rfind("approve_", 0) == 0 || query->data.rfind("decline_", 0) == 0 || query->data.rfind("status_", 0) == 0 || query->data.rfind("contact_user_", 0) == 0 || query->data.rfind("chat_start_", 0) == 0)
                                            {
                                                handle_admin_callbacks(bot, query);
                                            }
                                            return;
                                        }

                                        if (!db_get_bot_status())
                                        {
                                            bot.getApi().answerCallbackQuery(query->id, "Бот временно недоступен.");
                                            LOG(LogLevel::INFO, "Rejected callback from user " << chat_id << " because bot is inactive.");
                                            return;
                                        }

                                        if (query->data.rfind("admin_tp_", 0) == 0 || query->data.rfind("approve_", 0) == 0 || query->data.rfind("decline_", 0) == 0 || query->data.rfind("status_", 0) == 0 || query->data.rfind("contact_user_", 0) == 0 || query->data.rfind("chat_start_", 0) == 0)
                                        {
                                            handle_admin_callbacks(bot, query);
                                        }
                                        else
                                        {
                                            handle_client_callback(bot, query);
                                        }
                                    });

    try
    {
        LOG(LogLevel::INFO, "Bot username: " << bot.getApi().getMe()->username);
        LOG(LogLevel::INFO, "Bot started successfully. Press Ctrl+C to stop.");
        TgBot::TgLongPoll longPoll(bot);
        while (bot_running)
        {
            longPoll.start();
        }
        LOG(LogLevel::INFO, "Bot stopped gracefully.");
    }
    catch (const TgBot::TgException &e)
    {
        LOG(LogLevel::L_ERROR, "Telegram API ERROR: " << e.what());
    }

    // Stop HTTP server
    LOG(LogLevel::INFO, "Stopping HTTP API server...");
    if (httpServer)
    {
        httpServer->stop();
    }
    LOG(LogLevel::INFO, "HTTP API server stopped.");

    LOG(LogLevel::INFO, "Closing database...");
    db_close();
    LOG(LogLevel::INFO, "Shutdown complete.");
    return 0;
}
