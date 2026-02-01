#include "super_admin.h"
#include "main.h"
#include "admin_panel.h"
#include "config.h"
#include "database.h"
#include "application_flow.h"
#include "logger.h"
#include "trade_points.h"
#include "user_data_types.h"
#include <sstream>
#include <random>
#include <chrono>
#include <stdexcept>

class Timer
{
public:
    Timer(TgBot::Bot &bot, int64_t chat_id) : bot_(bot), chat_id_(chat_id), start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start_;
        std::stringstream ss;
        ss.precision(3);
        ss << "Операция выполнена за " << std::fixed << diff.count() << " сек.";
        LOG(LogLevel::INFO, "Timer for chat_id " << chat_id_ << ": " << ss.str());
        bot_.getApi().sendMessage(chat_id_, ss.str());
    }

private:
    TgBot::Bot &bot_;
    int64_t chat_id_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

void sendSuperAdminPanel(TgBot::Bot &bot, int64_t chat_id)
{
    admin_work_mode[chat_id] = AdminWorkMode::ADMIN_VIEW;
    db_save_admin_work_mode(chat_id, AdminWorkMode::ADMIN_VIEW);

    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    keyboard->resizeKeyboard = true;

    std::vector<TgBot::KeyboardButton::Ptr> row1;
    auto admin_panel_btn = std::make_shared<TgBot::KeyboardButton>();
    admin_panel_btn->text = "🖥️ Админ. панель";
    admin_panel_btn->webApp = std::make_shared<TgBot::WebAppInfo>();
    admin_panel_btn->webApp->url = "https://vld7wn.github.io/telegram_bot/webapp/admin/";
    row1.push_back(admin_panel_btn);
    keyboard->keyboard.push_back(row1);

    bot.getApi().sendMessage(chat_id, "👑 Добро пожаловать, Главный Администратор!", nullptr, nullptr, keyboard);
    LOG(LogLevel::INFO, "Sent super admin panel to chat ID: " << chat_id);
}

void sendAdminApprovalList(TgBot::Bot &bot, int64_t chat_id)
{
    admin_work_mode[chat_id] = AdminWorkMode::APPROVING_ADMINS;
    db_save_admin_work_mode(chat_id, AdminWorkMode::APPROVING_ADMINS);
    std::vector<AdminRequestData> requests = db_get_pending_admin_requests();

    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    keyboard->resizeKeyboard = true;
    auto back_btn = std::make_shared<TgBot::KeyboardButton>();
    back_btn->text = "⬅️ Назад";
    keyboard->keyboard.push_back({back_btn});

    if (requests.empty())
    {
        bot.getApi().sendMessage(chat_id, "Новых заявок на одобрение нет.", nullptr, nullptr, keyboard);
        LOG(LogLevel::INFO, "No pending admin requests for chat ID: " << chat_id);
        return;
    }

    bot.getApi().sendMessage(chat_id, "Выберите заявку для одобрения или отклонения:", nullptr, nullptr, keyboard);
    LOG(LogLevel::INFO, "Sent admin approval list to chat ID: " << chat_id << ". Found " << requests.size() << " requests.");

    for (const auto &req : requests)
    {
        std::stringstream text;
        text << "⚠️ *Новый запрос на доступ!*\n\n"
             << "👤 *Кандидат:* " << req.name << " (ID: `" << req.user_id << "`)\n"
             << "📍 *Торговая точка:* " << req.trade_point;

        auto inline_keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

        auto approve_btn = std::make_shared<TgBot::InlineKeyboardButton>();
        approve_btn->text = "✅ Одобрить";
        approve_btn->callbackData = "approve_" + std::to_string(req.user_id);

        auto decline_btn = std::make_shared<TgBot::InlineKeyboardButton>();
        decline_btn->text = "❌ Отклонить";
        decline_btn->callbackData = "decline_" + std::to_string(req.user_id);

        inline_keyboard->inlineKeyboard.push_back({approve_btn, decline_btn});

        bot.getApi().sendMessage(chat_id, text.str(), nullptr, nullptr, inline_keyboard, "Markdown");
    }
}

void sendAdminManagementPanel(TgBot::Bot &bot, int64_t chat_id)
{
    admin_work_mode[chat_id] = AdminWorkMode::SA_MANAGE_ADMINS;
    db_save_admin_work_mode(chat_id, AdminWorkMode::SA_MANAGE_ADMINS);

    std::vector<AdminRequestData> admins = db_get_all_admins();
    std::stringstream ss;
    ss << "Текущие администраторы:\n\n";
    if (admins.empty())
    {
        ss << "Список пуст.\n";
        LOG(LogLevel::INFO, "No active admins found for super admin (ID: " << chat_id << ").");
    }
    else
    {
        for (const auto &admin : admins)
        {
            ss << "👤 " << admin.name << " (ID: `" << admin.user_id << "`) - Точка: " << admin.trade_point << "\n";
        }
        LOG(LogLevel::INFO, "Sent admin management panel to super admin (ID: " << chat_id << "). Found " << admins.size() << " active admins.");
    }

    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    keyboard->resizeKeyboard = true;

    auto btn_add = std::make_shared<TgBot::KeyboardButton>();
    btn_add->text = "➕ Добавить админа";

    auto btn_del = std::make_shared<TgBot::KeyboardButton>();
    btn_del->text = "➖ Удалить админа";

    keyboard->keyboard.push_back({btn_add, btn_del});

    auto btn_back = std::make_shared<TgBot::KeyboardButton>();
    btn_back->text = "⬅️ Назад";

    keyboard->keyboard.push_back({btn_back});

    bot.getApi().sendMessage(chat_id, ss.str(), nullptr, nullptr, keyboard, "Markdown");
}

void sendTradePointSelectionForNewAdmin(TgBot::Bot &bot, int64_t chat_id)
{
    auto all_trade_points = get_all_trade_point_codes();

    if (all_trade_points.empty())
    {
        bot.getApi().sendMessage(chat_id, "В базе нет ни одной торговой точки. Добавление администратора невозможно.");
        sendAdminManagementPanel(bot, chat_id);
        return;
    }

    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    std::vector<TgBot::InlineKeyboardButton::Ptr> row;
    int buttons_per_row = 3;

    for (size_t i = 0; i < all_trade_points.size(); ++i)
    {
        auto btn = std::make_shared<TgBot::InlineKeyboardButton>();
        btn->text = all_trade_points[i];
        btn->callbackData = "add_admin_tp_select_" + all_trade_points[i];
        row.push_back(btn);

        if (row.size() == buttons_per_row || i == all_trade_points.size() - 1)
        {
            keyboard->inlineKeyboard.push_back(row);
            row.clear();
        }
    }

    auto back_btn = std::make_shared<TgBot::InlineKeyboardButton>();
    back_btn->text = "⬅️ Назад";
    back_btn->callbackData = "sa_back_to_manage_admins";
    keyboard->inlineKeyboard.push_back({back_btn});

    bot.getApi().sendMessage(chat_id, "Выберите торговую точку для нового администратора:", nullptr, nullptr, keyboard);
    LOG(LogLevel::INFO, "Sent trade point selection for new admin to chat ID: " << chat_id);
}

void sendAllApplicationsOverview(TgBot::Bot &bot, int64_t chat_id)
{
    admin_work_mode[chat_id] = AdminWorkMode::SA_AWAITING_TP_FOR_VIEW;
    db_save_admin_work_mode(chat_id, AdminWorkMode::SA_AWAITING_TP_FOR_VIEW);

    std::vector<std::string> codes = get_all_trade_point_codes();
    if (codes.empty())
    {
        bot.getApi().sendMessage(chat_id, "В базе нет ни одной торговой точки. Нечего просматривать.");
        sendSuperAdminPanel(bot, chat_id);
        return;
    }

    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    std::vector<TgBot::InlineKeyboardButton::Ptr> row;
    int buttons_per_row = 3;

    for (size_t i = 0; i < codes.size(); ++i)
    {
        auto btn = std::make_shared<TgBot::InlineKeyboardButton>();
        btn->text = codes[i];
        btn->callbackData = "sa_view_tp_" + codes[i];
        row.push_back(btn);

        if (row.size() == buttons_per_row || i == codes.size() - 1)
        {
            keyboard->inlineKeyboard.push_back(row);
            row.clear();
        }
    }

    auto back_btn = std::make_shared<TgBot::InlineKeyboardButton>();
    back_btn->text = "⬅️ Назад в панель ГА";
    back_btn->callbackData = "sa_back_to_panel";
    keyboard->inlineKeyboard.push_back({back_btn});

    bot.getApi().sendMessage(chat_id, "Выберите торговую точку для просмотра заявок:", nullptr, nullptr, keyboard);
    LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") entered 'view applications' mode.");
}

void handle_super_admin_message(TgBot::Bot &bot, TgBot::Message::Ptr message)
{
    int64_t chat_id = message->chat->id;
    const std::string &text = message->text;
    AdminWorkMode &current_mode = admin_work_mode[chat_id];

    LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") received message: " << text << " in mode: " << static_cast<int>(current_mode));

    if (text == "⬅️ Назад")
    {
        sendSuperAdminPanel(bot, chat_id);
        return;
    }
    if (text == "👀 Одобрение заявок админов")
    {
        LOG(LogLevel::INFO, "DEBUG: Matched 'Одобрение заявок админов'");
        sendAdminApprovalList(bot, chat_id);
        return;
    }
    if (text == "🕹️ Управление админами")
    {
        LOG(LogLevel::INFO, "DEBUG: Matched 'Управление админами'");
        sendAdminManagementPanel(bot, chat_id);
        return;
    }
    if (text == "🔴 Деактивировать бота")
    {
        db_set_bot_status(false);
        bot.getApi().sendMessage(chat_id, "Бот деактивирован. Он больше не будет обрабатывать запросы пользователей (кроме супер-админа).");
        sendSuperAdminPanel(bot, chat_id);
        LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") DEACTIVATED the bot.");
        return;
    }
    if (text == "🟢 Активировать бота")
    {
        db_set_bot_status(true);
        bot.getApi().sendMessage(chat_id, "Бот активирован. Он снова будет обрабатывать запросы пользователей.");
        sendSuperAdminPanel(bot, chat_id);
        LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") ACTIVATED the bot.");
        return;
    }
    if (text == "📊 Посмотреть заявки")
    {
        sendAllApplicationsOverview(bot, chat_id);
        return;
    }

    switch (current_mode)
    {
    case AdminWorkMode::ADMIN_VIEW:
        LOG(LogLevel::L_WARNING, "Super admin (ID: " << chat_id << ") sent unhandled message: " << text << " in ADMIN_VIEW mode.");
        break;

    case AdminWorkMode::APPROVING_ADMINS:
        break;

    case AdminWorkMode::SA_MANAGE_ADMINS:
        if (text == "➕ Добавить админа")
        {
            user_session_data[chat_id].state = UserState::AWAITING_ADMIN_TRADE_POINT_CHOICE;
            db_save_user_state(chat_id, user_session_data[chat_id].state);
            sendTradePointSelectionForNewAdmin(bot, chat_id);
            current_mode = AdminWorkMode::SA_AWAITING_ADD_TP;
            db_save_admin_work_mode(chat_id, current_mode);
            LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") started adding new admin.");
        }
        else if (text == "➖ Удалить админа")
        {
            current_mode = AdminWorkMode::SA_AWAITING_DELETE_ID;
            db_save_admin_work_mode(chat_id, current_mode);
            bot.getApi().sendMessage(chat_id, "Введите User ID администратора для удаления:");
            LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") started deleting admin.");
        }
        break;

    case AdminWorkMode::SA_AWAITING_ADD_ID:
        try
        {
            int64_t new_admin_id = std::stoll(text);
            std::string trade_point_code = user_session_data[chat_id].temp_trade_point_code;

            if (trade_point_code.empty())
            {
                bot.getApi().sendMessage(chat_id, "Ошибка: Торговая точка не была выбрана. Пожалуйста, начните заново, нажав '➕ Добавить админа'.");
                sendAdminManagementPanel(bot, chat_id);
                LOG(LogLevel::L_ERROR, "Super admin (ID: " << chat_id << ") tried to add admin without selecting trade point.");
                return;
            }

            if (db_admin_exists(new_admin_id))
            {
                bot.getApi().sendMessage(chat_id, "❌ Администратор с таким ID уже существует.");
                sendAdminManagementPanel(bot, chat_id);
                LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") attempted to add existing admin (ID: " << new_admin_id << ").");
                user_session_data[chat_id].temp_trade_point_code.clear();
                return;
            }

            db_add_admin_manual(new_admin_id, "Новый админ", trade_point_code);

            try
            {
                bot.getApi().sendMessage(new_admin_id, "Вас назначили администратором для точки *" + trade_point_code + "*. Отправьте /start, чтобы обновить меню.", nullptr, nullptr, nullptr, "Markdown");
                LOG(LogLevel::INFO, "Notified new admin (ID: " << new_admin_id << ") about assignment for TP: " << trade_point_code);
            }
            catch (const TgBot::TgException &e)
            {
                LOG(LogLevel::L_ERROR, "Failed to send notification to new admin " << new_admin_id << ": " << e.what());
            }

            bot.getApi().sendMessage(chat_id, "✅ Администратор (ID: " + std::to_string(new_admin_id) + ") успешно добавлен для торговой точки " + trade_point_code + ".");
            sendAdminManagementPanel(bot, chat_id);
            LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") added new admin (ID: " << new_admin_id << ") for trade point: " << trade_point_code);
            user_session_data[chat_id].temp_trade_point_code.clear();
        }
        catch (const std::exception &e)
        {
            bot.getApi().sendMessage(chat_id, "Неверный ID. Попробуйте снова. " + std::string(e.what()));
            LOG(LogLevel::L_ERROR, "Super admin (ID: " << chat_id << ") entered invalid ID for new admin: " << text << ". ERROR: " << e.what());
            sendAdminManagementPanel(bot, chat_id);
        }
        break;

    case AdminWorkMode::SA_AWAITING_ADD_TP:
        bot.getApi().sendMessage(chat_id, "Пожалуйста, выберите торговую точку из списка или нажмите '⬅️ Назад'.");
        break;

    case AdminWorkMode::SA_AWAITING_DELETE_ID:
        try
        {
            int64_t admin_to_delete = std::stoll(text);

            if (!db_admin_exists(admin_to_delete))
            {
                bot.getApi().sendMessage(chat_id, "❌ Администратор с таким ID не найден в списке одобренных админов.");
                sendAdminManagementPanel(bot, chat_id);
                LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") attempted to delete non-existent admin (ID: " << admin_to_delete << ").");
                return;
            }

            if (admin_to_delete == config.main_admin_id)
            {
                bot.getApi().sendMessage(chat_id, "❌ Вы не можете удалить самого себя из списка администраторов.");
                sendAdminManagementPanel(bot, chat_id);
                LOG(LogLevel::L_WARNING, "Super admin (ID: " << chat_id << ") attempted to delete self.");
                return;
            }

            try
            {
                bot.getApi().sendMessage(admin_to_delete, "Ваши права администратора были отозваны. Вы были возвращены в главное меню.");
                sendMainMenu(bot, admin_to_delete);
                LOG(LogLevel::INFO, "Notified removed admin (ID: " << admin_to_delete << ") about revocation of rights.");
            }
            catch (const TgBot::TgException &e)
            {
                LOG(LogLevel::L_ERROR, "Failed to notify removed admin " << admin_to_delete << ": " << e.what());
            }

            db_delete_admin(admin_to_delete);

            user_session_data.erase(admin_to_delete);
            admin_work_mode.erase(admin_to_delete);

            bot.getApi().sendMessage(chat_id, "✅ Администратор (ID: " + text + ") полностью удален.");
            sendAdminManagementPanel(bot, chat_id);
            LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") successfully deleted admin (ID: " << admin_to_delete << ").");
        }
        catch (const std::exception &e)
        {
            bot.getApi().sendMessage(chat_id, "Неверный ID или ошибка при удалении: " + std::string(e.what()));
            LOG(LogLevel::L_ERROR, "ERROR deleting admin " << text << " by super admin (ID: " << chat_id << "). ERROR: " << e.what());
            sendAdminManagementPanel(bot, chat_id);
        }
        break;
    case AdminWorkMode::SA_AWAITING_TP_FOR_VIEW:
        bot.getApi().sendMessage(chat_id, "Пожалуйста, выберите торговую точку из списка или нажмите '⬅️ Назад в панель ГА'.");
        break;
    default:
        LOG(LogLevel::L_WARNING, "Super admin (ID: " << chat_id << ") sent unhandled message: " << text << " in unknown mode: " << static_cast<int>(current_mode));
        break;
    }
}

void handle_super_admin_callbacks(TgBot::Bot &bot, TgBot::CallbackQuery::Ptr query)
{
    int64_t chat_id = query->message->chat->id;
    std::string callback_data = query->data;
    AdminWorkMode &current_mode = admin_work_mode[chat_id];

    if (callback_data.rfind("approve_", 0) == 0)
    {
        if (chat_id == config.main_admin_id)
        {
            int64_t new_admin_id = std::stoll(callback_data.substr(8));
            db_approve_admin(new_admin_id);
            bot.getApi().answerCallbackQuery(query->id, "Администратор одобрен!");

            std::string new_text = query->message->text + "\n\n*✅ ОДОБРЕН*";
            try
            {
                bot.getApi().editMessageText(new_text, chat_id, query->message->messageId, "", "Markdown");
                LOG(LogLevel::INFO, "Message edited successfully for approval of " << new_admin_id);
            }
            catch (const TgBot::TgException &e)
            {
                if (std::string(e.what()).find("message is not modified") != std::string::npos)
                {
                    LOG(LogLevel::L_WARNING, "Message for approval of " << new_admin_id << " was already modified: " << e.what());
                }
                else
                {
                    LOG(LogLevel::L_ERROR, "Telegram API Error editing message for approval of " << new_admin_id << ": " << e.what());
                }
            }

            std::string trade_point;
            db_is_admin_approved(new_admin_id, trade_point);
            try
            {
                bot.getApi().sendMessage(new_admin_id, "✅ Поздравляем! Ваш запрос на доступ одобрен для точки *" + trade_point + "*. Отправьте /start, чтобы обновить меню.", nullptr, nullptr, nullptr, "Markdown");
                LOG(LogLevel::INFO, "Notified new admin (ID: " << new_admin_id << ") about approval for TP: " << trade_point);
            }
            catch (const TgBot::TgException &e)
            {
                LOG(LogLevel::L_ERROR, "Failed to send approval notification to new admin " << new_admin_id << ": " << e.what());
            }
        }
        return;
    }

    if (callback_data.rfind("decline_", 0) == 0)
    {
        if (chat_id == config.main_admin_id)
        {
            int64_t new_admin_id = std::stoll(callback_data.substr(8));
            db_decline_admin_request(new_admin_id);
            bot.getApi().answerCallbackQuery(query->id, "Запрос отклонен.");

            std::string new_text = query->message->text + "\n\n*❌ ОТКЛОНЕН*";
            try
            {
                bot.getApi().editMessageText(new_text, chat_id, query->message->messageId, "", "Markdown");
                LOG(LogLevel::INFO, "Message edited successfully for rejection of " << new_admin_id);
            }
            catch (const TgBot::TgException &e)
            {
                if (std::string(e.what()).find("message is not modified") != std::string::npos)
                {
                    LOG(LogLevel::L_WARNING, "Message for rejection of " << new_admin_id << " was already modified: " << e.what());
                }
                else
                {
                    LOG(LogLevel::L_ERROR, "Telegram API Error editing message for rejection of " << new_admin_id << ": " << e.what());
                }
            }

            try
            {
                bot.getApi().sendMessage(new_admin_id, "Ваш запрос на доступ был отклонен.");
                LOG(LogLevel::INFO, "Notified new admin (ID: " << new_admin_id << ") about rejection.");
            }
            catch (const TgBot::TgException &e)
            {
                LOG(LogLevel::L_ERROR, "Failed to send rejection notification to new admin " << new_admin_id << ": " << e.what());
            }
        }
        return;
    }

    if (callback_data.rfind("add_admin_tp_select_", 0) == 0)
    {
        if (current_mode == AdminWorkMode::SA_AWAITING_ADD_TP)
        {
            std::string selected_trade_point = callback_data.substr(std::string("add_admin_tp_select_").length());
            user_session_data[chat_id].temp_trade_point_code = selected_trade_point;
            current_mode = AdminWorkMode::SA_AWAITING_ADD_ID;
            db_save_admin_work_mode(chat_id, current_mode);
            bot.getApi().answerCallbackQuery(query->id, "Выбрана точка: " + selected_trade_point);
            bot.getApi().sendMessage(chat_id, "Выбрана торговая точка: *" + selected_trade_point + "*. Теперь введите User ID нового администратора:", nullptr, nullptr, nullptr, "Markdown");
            LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") selected TP " << selected_trade_point << " for new admin.");
        }
        else
        {
            bot.getApi().answerCallbackQuery(query->id, "Неверное состояние для выбора торговой точки.", true);
            LOG(LogLevel::L_WARNING, "Super admin (ID: " << chat_id << ") tried to select TP in wrong mode: " << static_cast<int>(current_mode));
        }
        return;
    }

    if (callback_data.rfind("sa_view_tp_", 0) == 0)
    {
        if (current_mode == AdminWorkMode::SA_AWAITING_TP_FOR_VIEW)
        {
            std::string trade_point = callback_data.substr(std::string("sa_view_tp_").length());
            bot.getApi().answerCallbackQuery(query->id, "Загружаю заявки для " + trade_point + "...");

            sendApplicationsForReview(bot, chat_id, trade_point);

            admin_work_mode[chat_id] = AdminWorkMode::ADMIN_VIEW;
            db_save_admin_work_mode(chat_id, AdminWorkMode::ADMIN_VIEW);

            LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") viewed applications for TP: " << trade_point);
        }
        else
        {
            bot.getApi().answerCallbackQuery(query->id, "Неверное состояние для просмотра заявок по точке.", true);
        }
        return;
    }

    if (callback_data == "sa_back_to_panel")
    {
        bot.getApi().answerCallbackQuery(query->id);
        sendSuperAdminPanel(bot, chat_id);
        LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") returned to main super admin panel.");
        return;
    }

    if (callback_data == "sa_back_to_manage_admins")
    {
        bot.getApi().answerCallbackQuery(query->id);
        sendAdminManagementPanel(bot, chat_id);
        LOG(LogLevel::INFO, "Super admin (ID: " << chat_id << ") returned to admin management panel.");
        return;
    }
}
