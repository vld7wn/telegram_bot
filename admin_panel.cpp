#include "admin_panel.h"
#include "main.h"
#include "config.h"
#include "database.h"
#include "trade_points.h"
#include "application_flow.h"
#include "excel_generate.h"
#include "user_data_types.h"
#include "application_status.h"
#include "message_to_client.h"
#include <sstream>
#include <random>
#include <chrono>
#include <cstdio>
#include "logger.h"

// Вспомогательный класс для замера времени выполнения
class Timer {
public:
    Timer(TgBot::Bot& bot, int64_t chat_id) : bot_(bot), chat_id_(chat_id), start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start_;
        std::stringstream ss;
        ss.precision(3);
        ss << "Операция выполнена за " << std::fixed << diff.count() << " сек.";
        LOG(LogLevel::INFO, "Timer for chat_id " << chat_id_ << ": " << ss.str());
        bot_.getApi().sendMessage(chat_id_, ss.str());
    }
private:
    TgBot::Bot& bot_;
    int64_t chat_id_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// Обработка ввода имени администратора
void handle_admin_name_input_message(TgBot::Bot& bot, TgBot::Message::Ptr message) {
    int64_t chat_id = message->chat->id;
    UserData& user = user_session_data[chat_id];

    LOG(LogLevel::INFO, "Admin registration: Received name input '" << message->text << "' from ID " << chat_id);
    if (message->text.empty()) {
        bot.getApi().sendMessage(chat_id, "Имя не может быть пустым. Пожалуйста, введите ваше имя.", false, 0, nullptr, "");
        LOG(LogLevel::L_WARNING, "Admin registration: Empty name received from ID " << chat_id);
    } else {
        user.admin_name = message->text;
        user.state = UserState::AWAITING_APPROVAL;
        db_save_user_state(chat_id, user.state);
        send_approval_request(bot, chat_id, user.admin_name, user.admin_trade_point);
        LOG(LogLevel::INFO, "Admin registration: Request sent for name '" << user.admin_name << "', TP '" << user.admin_trade_point << "'");
    }
}

// Обработка ввода OTP
void handle_admin_otp_input_message(TgBot::Bot& bot, TgBot::Message::Ptr message) {
    int64_t chat_id = message->chat->id;
    UserData& user = user_session_data[chat_id];

    LOG(LogLevel::INFO, "Admin login: Received OTP input '" << message->text << "' from ID " << chat_id);
    Timer timer(bot, chat_id);
    if (admin_otps.count(chat_id) && admin_otps[chat_id] == message->text) {
        bot.getApi().sendMessage(chat_id, "Доступ разрешен. Добро пожаловать!");
        admin_otps.erase(chat_id);

        admin_work_mode[chat_id] = AdminWorkMode::ADMIN_VIEW;
        db_save_admin_work_mode(chat_id, AdminWorkMode::ADMIN_VIEW);

        sendAdminPanel(bot, chat_id);
        LOG(LogLevel::INFO, "Admin login: OTP successful for ID " << chat_id);
    } else {
        bot.getApi().sendMessage(chat_id, "Неверный пароль. Доступ запрещен.", false, 0, nullptr, "");
        sendMainMenu(bot, chat_id);
        LOG(LogLevel::L_WARNING, "Admin login: Incorrect OTP received from ID " << chat_id);
    }
}

// Обработка кнопок из админ-панели
void handle_admin_buttons_message(TgBot::Bot& bot, TgBot::Message::Ptr message) {
    int64_t chat_id = message->chat->id;
    std::string trade_point;

    LOG(LogLevel::INFO, "Admin panel: Received message '" << message->text << "' from ID " << chat_id);

    if (!db_is_admin_approved(chat_id, trade_point)) {
        bot.getApi().sendMessage(chat_id, "Ошибка доступа. Пожалуйста, вернитесь в главное меню.", false, 0, nullptr, "");
        sendMainMenu(bot, chat_id);
        LOG(LogLevel::L_ERROR, "Admin panel: Access denied for ID " << chat_id);
        return;
    }

    if (message->text == "Посмотреть заявки") {
        sendApplicationsForReview(bot, chat_id, trade_point);
        LOG(LogLevel::INFO, "Admin panel: Viewing applications for TP '" << trade_point << "' by ID " << chat_id);
    } else if (message->text == "Выгрузить в Excel") {
        bot.getApi().sendMessage(chat_id, "Начинаю генерацию отчета...");
        Timer timer(bot, chat_id);
        try {
            std::string report_path = generate_excel_report(trade_point);
            if (!report_path.empty()) {
                bot.getApi().sendMessage(chat_id, "Отчет готов! Отправляю файл...", false, 0, nullptr, "");
                bot.getApi().sendDocument(chat_id, TgBot::InputFile::fromFile(report_path, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
                remove(report_path.c_str());
                LOG(LogLevel::INFO, "Admin panel: Excel report generated and sent for TP '" << trade_point << "' by ID " << chat_id);
            } else {
                 bot.getApi().sendMessage(chat_id, "Не удалось создать отчет.", false, 0, nullptr, "");
                 LOG(LogLevel::L_ERROR, "Admin panel: Failed to generate Excel report for TP '" << trade_point << "' by ID " << chat_id);
            }
        } catch (const std::exception& e) {
            bot.getApi().sendMessage(chat_id, std::string("Ошибка при создании отчета: ") + e.what(), false, 0, nullptr, "");
            LOG(LogLevel::L_ERROR, "Admin panel: Exception during Excel report generation for TP '" << trade_point << "' by ID " << chat_id << ": " << e.what());
        }
    } else if (message->text == "Выход из панели") {
        sendMainMenu(bot, chat_id);
        LOG(LogLevel::INFO, "Admin panel: Exiting panel for ID " << chat_id);
    } else {
        bot.getApi().sendMessage(chat_id, "Неизвестная команда. Пожалуйста, используйте кнопки на клавиатуре.", false, 0, nullptr, "");
        LOG(LogLevel::L_WARNING, "Admin panel: Unhandled button message '" << message->text << "' from ID " << chat_id);
    }
}

void handle_admin_panel_message(TgBot::Bot& bot, TgBot::Message::Ptr message) {
    int64_t chat_id = message->chat->id;
    UserData& user = user_session_data[chat_id];

    LOG(LogLevel::INFO, "handle_admin_panel_message called for ID " << chat_id << " in state: " << static_cast<int>(user.state));

    switch (user.state) {
        case UserState::ADMIN_REPLYING_TO_USER:
            handle_client_reply(bot, message);
            return;
        case UserState::AWAITING_ADMIN_NAME:
            handle_admin_name_input_message(bot, message);
            return;
        case UserState::AWAITING_ADMIN_PASSWORD:
            handle_admin_otp_input_message(bot, message);
            return;
        case UserState::ADMIN_PANEL:
            handle_admin_buttons_message(bot, message);
            return;
        default:
            LOG(LogLevel::L_WARNING, "Admin panel: Unhandled message '" << message->text << "' from ID " << chat_id << " in unexpected state: " << static_cast<int>(user.state));
            break;
    }
}

void sendAdminPanel(TgBot::Bot& bot, int64_t chat_id) {
    user_session_data[chat_id].state = UserState::ADMIN_PANEL;
    db_save_user_state(chat_id, user_session_data[chat_id].state);

    admin_work_mode[chat_id] = AdminWorkMode::ADMIN_VIEW;
    db_save_admin_work_mode(chat_id, AdminWorkMode::ADMIN_VIEW);

    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    keyboard->resizeKeyboard = true;

    std::vector<TgBot::KeyboardButton::Ptr> row1;
    auto view_btn = std::make_shared<TgBot::KeyboardButton>();
    view_btn->text = "Посмотреть заявки";
    row1.push_back(view_btn);
    keyboard->keyboard.push_back(row1);

    std::vector<TgBot::KeyboardButton::Ptr> row2;
    auto excel_btn = std::make_shared<TgBot::KeyboardButton>();
    excel_btn->text = "Выгрузить в Excel";
    row2.push_back(excel_btn);
    keyboard->keyboard.push_back(row2);

    std::vector<TgBot::KeyboardButton::Ptr> row3;
    auto logout_btn = std::make_shared<TgBot::KeyboardButton>();
    logout_btn->text = "Выход из панели";
    row3.push_back(logout_btn);
    keyboard->keyboard.push_back(row3);

    bot.getApi().sendMessage(chat_id, "👑 Панель администратора", false, 0, keyboard);
    LOG(LogLevel::INFO, "Admin panel sent to ID " << chat_id);
}


void sendApplicationsForReview(TgBot::Bot& bot, int64_t chat_id, const std::string& trade_point) {
    auto requests = db_get_apps_data_for_report(trade_point);
    LOG(LogLevel::INFO, "Found " << requests.size() << " applications for TP '" << trade_point << "'");

    if (requests.empty()) {
        bot.getApi().sendMessage(chat_id, "Для вашей точки нет новых заявок.", false, 0, nullptr, "");
        return;
    }

    bot.getApi().sendMessage(chat_id, "Последние заявки для точки " + trade_point + ":", false, 0, nullptr, "");

    for(const auto& req : requests) {
        std::stringstream text;
        text << "---------------------\n"
             << "*Заявка №" << req.id << "*\n"
             << "Клиент: " << req.name << " (ID: `" << req.user_id << "`)\n"
             << "Телефон: " << req.phone;

        text << "Статус чата: " << req.chat_status << "\n";

        auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

        std::vector<TgBot::InlineKeyboardButton::Ptr> row1;
        auto btn_progress = std::make_shared<TgBot::InlineKeyboardButton>();
        btn_progress->text = "Взять в работу";
        btn_progress->callbackData = "status_progress_" + std::to_string(req.id) + "_" + std::to_string(req.user_id);
        row1.push_back(btn_progress);

        auto btn_done = std::make_shared<TgBot::InlineKeyboardButton>();
        btn_done->text = "Выполнена";
        btn_done->callbackData = "status_done_" + std::to_string(req.id) + "_" + std::to_string(req.user_id);
        row1.push_back(btn_done);

        std::vector<TgBot::InlineKeyboardButton::Ptr> row2;
        auto btn_cancel = std::make_shared<TgBot::InlineKeyboardButton>();
        btn_cancel->text = "Отменить";
        btn_cancel->callbackData = "status_cancel_" + std::to_string(req.id) + "_" + std::to_string(req.user_id);
        row2.push_back(btn_cancel);

        auto btn_contact = std::make_shared<TgBot::InlineKeyboardButton>();
        btn_contact->text = "Начать переписку";
        btn_contact->callbackData = "chat_start_" + std::to_string(req.id) + "_" + std::to_string(req.user_id);
        row2.push_back(btn_contact);

        keyboard->inlineKeyboard.push_back(row1);
        keyboard->inlineKeyboard.push_back(row2);

        bot.getApi().sendMessage(chat_id, text.str(), false, 0, keyboard, "Markdown");
    }
}

void start_admin_registration(TgBot::Bot& bot, int64_t chat_id) {
    user_session_data[chat_id].state = UserState::AWAITING_ADMIN_TRADE_POINT_CHOICE;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    std::vector<std::string> codes = get_all_trade_point_codes();
    LOG(LogLevel::INFO, "Started admin registration for ID " << chat_id << ". Found " << codes.size() << " trade points.");

    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    std::vector<TgBot::InlineKeyboardButton::Ptr> row;

    for (size_t i = 0; i < codes.size(); ++i) {
        auto btn = std::make_shared<TgBot::InlineKeyboardButton>();
        btn->text = codes[i];
        btn->callbackData = "admin_tp_" + codes[i];
        row.push_back(btn);

        if (row.size() == 3 || i == codes.size() - 1) {
            keyboard->inlineKeyboard.push_back(row);
            row.clear();
        }
    }
    bot.getApi().sendMessage(chat_id, "Вы не являетесь администратором. Для получения доступа выберите вашу торговую точку:", false, 0, keyboard);
}

void send_approval_request(TgBot::Bot& bot, int64_t new_admin_id, const std::string& name, const std::string& trade_point) {
    db_add_admin_request(new_admin_id, name, trade_point);
    bot.getApi().sendMessage(new_admin_id, "Ваш запрос отправлен на рассмотрение главному администратору. Пожалуйста, ожидайте.", false, 0, nullptr, "");
    LOG(LogLevel::INFO, "Sent approval request for new admin '" << name << "' (ID: " << new_admin_id << ") for TP '" << trade_point << "'");
}

void send_otp(TgBot::Bot& bot, int64_t admin_id, const std::string& reason) {
    std::string otp = std::to_string(std::mt19937(std::random_device()())() % 900000 + 100000);
    admin_otps[admin_id] = otp;
    LOG(LogLevel::INFO, "Generated OTP '" << otp << "' for admin ID " << admin_id << " for reason: " << reason);

    std::stringstream text;
    text << "🔑 *Пароль для " << reason << " администратора (ID: `" << admin_id << "`)*\n\n"
         << "Пароль: `" << otp << "`";

    bot.getApi().sendMessage(config.main_admin_id, text.str(), false, 0, nullptr, "Markdown");
    bot.getApi().sendMessage(admin_id, "Пароль для входа был отправлен главному администратору. Пожалуйста, введите его:", false, 0, nullptr, "");
}

void handle_admin_callbacks(TgBot::Bot& bot, TgBot::CallbackQuery::Ptr query) {
    int64_t chat_id = query->message->chat->id;
    int32_t message_id = query->message->messageId;
    UserData& user = user_session_data[chat_id];

    LOG(LogLevel::INFO, "handle_admin_callbacks called for ID " << chat_id << ", callback data: " << query->data);

    if (query->data.rfind("admin_tp_", 0) == 0) {
        user.admin_trade_point = query->data.substr(9);
        user.state = UserState::AWAITING_ADMIN_NAME;
        db_save_user_state(chat_id, user.state);
        bot.getApi().answerCallbackQuery(query->id);
        bot.getApi().editMessageText("Точка " + user.admin_trade_point + " выбрана. Теперь введите ваше имя:", chat_id, message_id);
        LOG(LogLevel::INFO, "Admin registration: TP selected '" << user.admin_trade_point << "' by ID " << chat_id);
        return;
    }

    if (query->data.rfind("status_", 0) == 0) {
        std::string data = query->data.substr(7);
        size_t pos1 = data.find('_');
        size_t pos2 = data.find('_', pos1 + 1);
        std::string status_type = data.substr(0, pos1);
        long long app_id = std::stoll(data.substr(pos1 + 1, pos2 - (pos1 + 1)));
        int64_t user_id_client = std::stoll(data.substr(pos2 + 1));
        std::string status_text;
        ApplicationStatus new_status;
        if (status_type == "progress") {
            new_status = ApplicationStatus::InProgress;
            status_text = "В работе";
        } else if (status_type == "done") {
            new_status = ApplicationStatus::Done;
            status_text = "Выполнена";
        } else if (status_type == "cancel") {
            new_status = ApplicationStatus::Cancelled;
            status_text = "Отменена";
        } else {
            return;
        }
        db_update_application_status(app_id, new_status);
        bot.getApi().answerCallbackQuery(query->id, "Статус обновлен: " + status_text);
        std::string new_text = query->message->text + "\n\n*✅ Статус обновлен на '" + status_text + "'*";
        try {
            bot.getApi().editMessageText(new_text, query->message->chat->id, query->message->messageId);
        } catch (const TgBot::TgException& e) {
            if (std::string(e.what()).find("message is not modified") == std::string::npos) {
                LOG(LogLevel::L_ERROR, "Error editing message status: " << e.what());
            } else {
                LOG(LogLevel::L_WARNING, "Message status already modified or no change: " << e.what());
            }
        }
        bot.getApi().sendMessage(user_id_client, "📈 Статус вашей заявки №" + std::to_string(app_id) + " изменился: *" + status_text + "*.", false, 0, nullptr, "Markdown");
        LOG(LogLevel::INFO, "Status for app " << app_id << " updated to '" << status_text << "' by admin ID " << chat_id);
        return;
    }

    if (query->data.rfind("chat_start_", 0) == 0) {
        bot.getApi().answerCallbackQuery(query->id);

        std::string data_str = query->data.substr(11);
        size_t first_underscore = data_str.find('_');

        long long app_id = std::stoll(data_str.substr(0, first_underscore));
        int64_t user_to_contact_id = std::stoll(data_str.substr(first_underscore + 1));

        LOG(LogLevel::INFO, "Admin (ID: " << chat_id << ") clicked 'chat_start_'. Parsed client ID: " << user_to_contact_id << ", App ID: " << app_id);
        user.reply_to_user_id = user_to_contact_id;
        user.current_application_id = app_id;

        std::string admin_name = user.admin_name;
        std::string trade_point = user.admin_trade_point;
        if (admin_name.empty() || trade_point.empty()) {
            std::string temp_trade_point;
            if (db_is_admin_approved(chat_id, temp_trade_point)) {
                trade_point = temp_trade_point;
            }
        }

        db_update_chat_status(app_id, ChatStatus::InProgress, chat_id);

        user.state = UserState::ADMIN_REPLYING_TO_USER;
        db_save_user_state(chat_id, user.state);
        auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
        keyboard->resizeKeyboard = true;
        auto cancel_btn = std::make_shared<TgBot::KeyboardButton>();
        cancel_btn->text = "Отмена";
        keyboard->keyboard.push_back({cancel_btn});
        bot.getApi().sendMessage(chat_id, "Введите сообщение, которое хотите отправить клиенту (ID: " + std::to_string(user_to_contact_id) + ").", false, 0, keyboard);
        return;
    }

    if (query->data.rfind("chat_status_", 0) == 0) {
        std::string data_str = query->data.substr(12);
        size_t first_underscore = data_str.find('_');
        long long app_id = std::stoll(data_str.substr(0, first_underscore));
        std::string action = data_str.substr(first_underscore + 1);

        LOG(LogLevel::INFO, "Admin (ID: " << chat_id << ") changed chat status for app ID " << app_id << " to " << action);

        if (action == "completed") {
            db_update_chat_status(app_id, ChatStatus::Completed);
            bot.getApi().answerCallbackQuery(query->id, "Диалог завершен.");
            bot.getApi().sendMessage(user.reply_to_user_id, "💬 Диалог по вашей заявке №" + std::to_string(app_id) + " был завершен.", false, 0, nullptr, "");
            std::string trade_point;
            if (db_is_admin_approved(chat_id, trade_point)) {
                sendApplicationsForReview(bot, chat_id, trade_point);
            }
        }
        return;
    }
}

void notifyAdminsOfNewApplication(TgBot::Bot& bot, const std::string& trade_point, const std::string& message) {
    std::vector<int64_t> admin_ids = db_get_admin_ids_by_trade_point(trade_point);
    admin_ids.push_back(config.main_admin_id);
    std::string notification = "🔔 Новая заявка для точки *" + trade_point + "*!\n\n" + message;
    for (int64_t admin_id : admin_ids) {
        try {
            bot.getApi().sendMessage(admin_id, notification, false, 0, nullptr, "Markdown");
        } catch (const TgBot::TgException& e) {
            LOG(LogLevel::L_ERROR, "Failed to send notification to admin " << admin_id << ": " << e.what());
        }
    }
}
