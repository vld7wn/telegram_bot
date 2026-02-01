#include "application_flow.h"
#include "main.h"
#include "database.h"
#include "trade_points.h"
#include "utils.h"
#include "config.h"
#include "admin_panel.h"
#include "faq_manager.h"
#include "tariff_manager.h"
#include "user_data_types.h"
#include "application_status.h"
#include "message_to_client.h"
#include "state_handler.h"
#include <sstream>
#include <algorithm>
#include "logger.h"

// Обработка кнопок главного меню.
bool handle_main_menu_buttons(TgBot::Bot &bot, TgBot::Message::Ptr message)
{
    int64_t chat_id = message->chat->id;
    UserData &user = user_session_data[chat_id];

    if (user.state != UserState::NONE)
    {
        return false;
    }

    if (message->text == "📝 Оставить заявку")
    {
        user_session_data[chat_id] = UserData();
        db_save_user_state(chat_id, user_session_data[chat_id].state);
        sendTradePointSelection(bot, chat_id);
        return true;
    }
    if (message->text == "📂 Мои заявки")
    {
        std::string my_apps = db_get_my_apps(chat_id);
        bot.getApi().sendMessage(chat_id, my_apps, nullptr, nullptr, nullptr, "Markdown");
        return true;
    }
    if (message->text == "🌐 Проверить возможность подключения")
    {
        user.state = UserState::AWAITING_ADDRESS_FOR_CHECK;
        db_save_user_state(chat_id, user.state);

        auto removal_keyboard = std::make_shared<TgBot::ReplyKeyboardRemove>();
        bot.getApi().sendMessage(chat_id, "Убираю меню...", nullptr, nullptr, removal_keyboard, "Markdown", true);
        sendBackButtonKeyboard(bot, chat_id, "Пожалуйста, введите ваш полный адрес для проверки (город, улица, дом, корпус, квартира):");
        return true;
    }
    if (message->text == "❓ Помощь")
    {
        sendHelpMenu(bot, message->chat->id);
        return true;
    }
    if (message->text == "РТК")
    {
        std::string trade_point;
        if (db_is_admin_approved(chat_id, trade_point))
        {
            bot.getApi().sendMessage(chat_id, "Вы уже являетесь администратором.", nullptr, nullptr, nullptr, "");
        }
        else
        {
            start_admin_registration(bot, chat_id);
        }
        return true;
    }
    if (message->text == "👑 Панель администратора" && chat_id != config.main_admin_id)
    {
        std::string trade_point;
        if (db_is_admin_approved(chat_id, trade_point))
        {
            user.state = UserState::AWAITING_ADMIN_PASSWORD;
            db_save_user_state(chat_id, user.state);
            user.admin_trade_point = trade_point;
            send_otp(bot, chat_id, "входа в панель");
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "У вас нет прав администратора. Если вы хотите стать администратором, нажмите 'РТК'.", nullptr, nullptr, nullptr, "");
        }
        return true;
    }
    if (message->text == "📞 Связаться с сотрудником")
    {
        std::stringstream ss;
        ss << "📞 *Запрос на обратный звонок*\n\n"
           << "👤 *Пользователь:* " << message->from->firstName << " " << message->from->lastName
           << " (ID: `" << chat_id << "`)\n"
           << "Пользователь хочет связаться с сотрудником.";
        bot.getApi().sendMessage(config.main_admin_id, ss.str(), nullptr, nullptr, nullptr, "Markdown");
        bot.getApi().sendMessage(chat_id, "✅ Ваш запрос отправлен! Сотрудник свяжется с вами в ближайшее время.");
        sendPostApplicationMenu(bot, chat_id);
        return true;
    }

    return false;
}

// Обработка сообщений от клиента в зависимости от состояния.
void handle_client_message(TgBot::Bot &bot, TgBot::Message::Ptr message)
{
    int64_t chat_id = message->chat->id;
    UserData &user = user_session_data[chat_id];

    bool is_bot_active = db_get_bot_status();
    bool is_super_admin = (chat_id == config.main_admin_id);

    if (!is_bot_active && !is_super_admin)
    {
        bot.getApi().sendMessage(chat_id, "Бот временно недоступен для обработки запросов. Пожалуйста, попробуйте позже.");
        LOG(LogLevel::INFO, "Rejected message from user " << chat_id << " because bot is inactive.");
        return;
    }

    // Сохранение сообщения в историю чата, если есть активная заявка
    if (!message->text.empty() && message->text[0] != '/')
    {
        int64_t app_id = db_get_latest_application_id(chat_id);
        if (app_id > 0)
        {
            ChatMessage msg;
            msg.sender = "client";
            msg.text = message->text;
            db_add_chat_message(app_id, msg);
            LOG(LogLevel::INFO, "Client message saved to chat history for application " + std::to_string(app_id));
        }
    }

    if (message->text == "⬅️ Назад")
    {
        switch (user.state)
        {
        case UserState::VIEWING_TARIFF_DETAILS:
        case UserState::CHOOSING_TV:
        case UserState::ENTERING_NAME:
            if (!user.selected_speed_option.value.empty())
            {
                TgBot::InlineKeyboardMarkup::Ptr speed_keyboard = create_tariff_speed_buttons(user.selected_tariff.id);
                if (speed_keyboard)
                {
                    bot.getApi().sendMessage(chat_id, user.selected_tariff.get_tariff_description() + "\nВыберите желаемую скорость:", nullptr, nullptr, nullptr, "Markdown", true);
                    user.state = UserState::VIEWING_TARIFF_DETAILS;
                    db_save_user_state(chat_id, user.state);
                }
                else
                {
                    sendTariffSelection(bot, chat_id);
                }
            }
            else
            {
                sendTariffSelection(bot, chat_id);
            }
            break;
        case UserState::ENTERING_PHONE:
            askForName(bot, chat_id);
            break;
        case UserState::CHOOSING_MESSENGER:
            askForPhone(bot, chat_id);
            break;
        case UserState::ENTERING_EMAIL:
            askForMessenger(bot, chat_id);
            break;
        case UserState::ENTERING_CITY:
            askForEmail(bot, chat_id);
            break;
        case UserState::ENTERING_STREET:
            askForCity(bot, chat_id);
            break;
        case UserState::ENTERING_HOUSE:
            askForStreet(bot, chat_id);
            break;
        case UserState::ENTERING_HOUSE_BODY:
            askForHouse(bot, chat_id);
            break;
        case UserState::ENTERING_APARTMENT:
            askForHouseBody(bot, chat_id);
            break;
        default:
            sendMainMenu(bot, chat_id);
            break;
        }
        return;
    }

    if (user.state == UserState::HELP_SECTION)
    {
        if (message->text == "⬅️ Назад в главное меню")
        {
            sendMainMenu(bot, chat_id);
        }
        return;
    }

    switch (user.state)
    {
    case UserState::AWAITING_ADDRESS_FOR_CHECK:
    {
        std::string address = message->text;
        std::stringstream ss;
        ss << "❗️*Запрос на проверку адреса!*\n\n"
           << "👤 *Пользователь:* " << message->from->firstName << " " << message->from->lastName
           << " (ID: `" << chat_id << "`)\n"
           << "🏠 *Адрес:* " << address;
        bot.getApi().sendMessage(config.main_admin_id, ss.str(), nullptr, nullptr, nullptr, "Markdown");
        bot.getApi().sendMessage(chat_id, "Спасибо! Ваш запрос на проверку адреса отправлен. Администратор свяжется с вами для уточнения деталей.");
        sendMainMenu(bot, chat_id);
        break;
    }
    case UserState::ENTERING_NAME:
        if (isNotEmpty(message->text))
        {
            user.name = message->text;
            askForPhone(bot, chat_id);
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Имя не может быть пустым.");
        }
        break;
    case UserState::ENTERING_PHONE:
    {
        std::string phone_number = (message->contact != nullptr) ? message->contact->phoneNumber : message->text;
        if (isValidPhone(phone_number))
        {
            user.phone = phone_number;
            askForMessenger(bot, chat_id);
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Неверный формат. Введите 10 цифр или отправьте контакт.\nПример: 912 345 67 89");
        }
        break;
    }
    case UserState::ENTERING_EMAIL:
        if (isValidEmail(message->text))
        {
            user.email = message->text;
            askForCity(bot, chat_id);
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Неверный формат почты (например, user@example.com).");
        }
        break;
    case UserState::ENTERING_CITY:
        if (isNotEmpty(message->text))
        {
            user.city = message->text;
            askForStreet(bot, chat_id);
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Название города не может быть пустым.");
        }
        break;
    case UserState::ENTERING_STREET:
        if (isNotEmpty(message->text))
        {
            user.street = message->text;
            askForHouse(bot, chat_id);
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Название улицы не может быть пустым.");
        }
        break;
    case UserState::ENTERING_HOUSE:
        if (isNotEmpty(message->text))
        {
            user.house = message->text;
            askForHouseBody(bot, chat_id);
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Номер дома не может быть пустым.");
        }
        break;
    case UserState::ENTERING_HOUSE_BODY:
        if (message->text != "Пропустить")
        {
            user.house_body = message->text;
        }
        else
        {
            user.house_body = "";
        }
        askForApartment(bot, chat_id);
        break;
    case UserState::ENTERING_APARTMENT:
    {
        if (message->text != "Пропустить")
        {
            user.apartment = message->text;
        }
        else
        {
            user.apartment = "не указана";
        }

        int total_monthly = std::stoi(user.selected_speed_option.price);
        std::string rent_details = "";

        if (!user.selected_tariff.router_rental.empty())
        {
            int router_cost = 0;
            try
            {
                router_cost = std::stoi(user.selected_tariff.router_rental.substr(0, user.selected_tariff.router_rental.find(' ')));
            }
            catch (const std::exception &e)
            {
                LOG(LogLevel::L_ERROR, "Failed to parse router_rental price: " << user.selected_tariff.router_rental << " Error: " << e.what());
            }
            total_monthly += router_cost;
            rent_details += user.selected_tariff.router_rental + " (роутер)";
        }
        if (user.needs_tv_box && !user.selected_tariff.tv_box_rental.empty())
        {
            int tv_box_cost = 0;
            try
            {
                tv_box_cost = std::stoi(user.selected_tariff.tv_box_rental.substr(0, user.selected_tariff.tv_box_rental.find(' ')));
            }
            catch (const std::exception &e)
            {
                LOG(LogLevel::L_ERROR, "Failed to parse tv_box_rental price: " << user.selected_tariff.tv_box_rental << " Error: " << e.what());
            }
            total_monthly += tv_box_cost;
            if (!rent_details.empty())
                rent_details += " + ";
            rent_details += user.selected_tariff.tv_box_rental + " (ТВ-приставка)";
        }
        if (rent_details.empty())
        {
            rent_details = "Нет";
        }

        int connection_fee = 0;
        if (!user.selected_tariff.connection_fee.empty())
        {
            try
            {
                connection_fee = std::stoi(user.selected_tariff.connection_fee.substr(0, user.selected_tariff.connection_fee.find(' ')));
            }
            catch (const std::exception &e)
            {
                LOG(LogLevel::L_ERROR, "Failed to parse connection_fee price: " << user.selected_tariff.connection_fee << " Error: " << e.what());
            }
        }

        std::stringstream client_invoice;
        client_invoice << "*Ваша заявка сформирована:*\n\n"
                       << "👤 *Имя:* " << user.name << "\n"
                       << "📡 *Тариф:* " << user.final_tariff_string << "\n"
                       << "🏠 *Адрес:* "
                       << "г. " << user.city << ", ул. " << user.street << ", д. " << user.house;
        if (!user.house_body.empty())
            client_invoice << ", корп. " << user.house_body;
        if (user.apartment != "не указана")
            client_invoice << ", кв. " << user.apartment;
        client_invoice << "\n\n*Расчет стоимости:*\n"
                       << "Ежемесячная плата по тарифу: *" << user.selected_speed_option.price << " ₽*\n";
        if (!user.selected_speed_option.promo_price_duration_months.empty())
        {
            client_invoice << "(акция: " << user.selected_speed_option.promo_price_duration_months << " мес, далее " << user.selected_speed_option.full_price << " ₽)\n";
        }
        client_invoice << "Аренда оборудования: *" << rent_details << "*\n"
                       << "*Итого к оплате ежемесячно: " << total_monthly << " ₽*\n\n"
                       << "*Единоразовый платеж за подключение: " << connection_fee << " ₽*";
        bot.getApi().sendMessage(chat_id, client_invoice.str(), nullptr, nullptr, nullptr, "Markdown");

        std::string full_address = "г. " + user.city + ", ул. " + user.street + ", д. " + user.house;
        if (!user.house_body.empty())
        {
            full_address += ", корп. " + user.house_body;
        }
        if (user.apartment != "не указана")
        {
            full_address += ", кв. " + user.apartment;
        }

        db_add_application(chat_id, user, full_address, total_monthly);

        std::stringstream notification_text;
        notification_text << "Имя: " << user.name << "\n"
                          << "Телефон: " << user.phone << "\n"
                          << "Тариф: " << user.final_tariff_string;
        notifyAdminsOfNewApplication(bot, user.flyer_code, notification_text.str());

        std::string final_message = "Ваша заявка принята. Скоро с вами свяжутся для уточнения деталей.\n\n"
                                    "Для подключения интернета подойдите по адресу:\n*" +
                                    user.office_address + "*\n\n"
                                                          "**Не забудьте взять с собой паспорт!**";
        bot.getApi().sendMessage(chat_id, final_message, nullptr, nullptr, nullptr, "Markdown");
        sendMainMenu(bot, chat_id);
        break;
    }
    default:
        break;
    }
}

void handle_client_callback(TgBot::Bot &bot, TgBot::CallbackQuery::Ptr query)
{
    int64_t chat_id = query->message->chat->id;
    int32_t message_id = query->message->messageId;
    UserData &user = user_session_data[chat_id];

    bool is_bot_active = db_get_bot_status();
    bool is_super_admin = (chat_id == config.main_admin_id);

    if (!is_bot_active && !is_super_admin)
    {
        bot.getApi().answerCallbackQuery(query->id, "Бот временно недоступен.");
        LOG(LogLevel::INFO, "Rejected callback from user " << chat_id << " because bot is inactive.");
        return;
    }

    if (query->data.rfind("flyer_code_", 0) == 0)
    {
        LOG(LogLevel::INFO, "Attempting to handle flyer_code_ callback. Current user state: " << static_cast<int>(user.state));
        if (user.state != UserState::CHOOSING_FLYER_CODE)
        {
            LOG(LogLevel::L_WARNING, "Client callback 'flyer_code_' received but user state is not CHOOSING_FLYER_CODE. State: " << static_cast<int>(user.state));
            bot.getApi().answerCallbackQuery(query->id, "Пожалуйста, начните процесс заявки заново через главное меню.", true);
            return;
        }
        std::string code = query->data.substr(11);
        std::string address;
        if (get_address_by_code(code, address))
        {
            LOG(LogLevel::INFO, "Trade point found for code: " << code << ", Address: " << address);
            user.flyer_code = code;
            user.office_address = address;
            bot.getApi().answerCallbackQuery(query->id);
            bot.getApi().editMessageText("✅ Код точки " + code + " принят.\nВаш офис: *" + address + "*", chat_id, message_id, "", "Markdown", nullptr, nullptr);
            sendTariffSelection(bot, chat_id);
            LOG(LogLevel::INFO, "Called sendTariffSelection for chat ID: " << chat_id);
        }
        else
        {
            LOG(LogLevel::L_ERROR, "Trade point not found for code: " << code);
            bot.getApi().answerCallbackQuery(query->id, "Ошибка! Код не найден в базе. Попробуйте снова.", true);
        }
        return;
    }

    if (query->data.rfind("show_tariff_detail_", 0) == 0)
    {
        if (user.state != UserState::CHOOSING_TARIFF)
            return;
        bot.getApi().answerCallbackQuery(query->id);

        std::string tariff_id = query->data.substr(std::string("show_tariff_detail_").length());
        TariffPlan selected_tariff = get_tariff_by_id(tariff_id);

        if (selected_tariff.id.empty())
        {
            bot.getApi().sendMessage(chat_id, "Ошибка: тариф не найден.", nullptr, nullptr, nullptr, "");
            sendTariffSelection(bot, chat_id);
            return;
        }

        user.selected_tariff = selected_tariff;

        TgBot::InlineKeyboardMarkup::Ptr speed_keyboard = create_tariff_speed_buttons(tariff_id);

        bot.getApi().editMessageText(selected_tariff.get_tariff_description(), chat_id, message_id, "", "Markdown", nullptr, speed_keyboard);
        user.state = UserState::VIEWING_TARIFF_DETAILS;
        db_save_user_state(chat_id, user.state);
        LOG(LogLevel::INFO, "User (ID: " << chat_id << ") is viewing details for tariff: " << selected_tariff.name);
        return;
    }

    if (query->data.rfind("select_speed_", 0) == 0)
    {
        if (user.state != UserState::VIEWING_TARIFF_DETAILS)
            return;
        bot.getApi().answerCallbackQuery(query->id);

        std::string data_str = query->data.substr(std::string("select_speed_").length());
        size_t first_underscore = data_str.find('_');
        size_t second_underscore = data_str.find('_', first_underscore + 1);

        std::string tariff_id = data_str.substr(0, first_underscore);
        std::string speed_value = data_str.substr(first_underscore + 1, second_underscore - (first_underscore + 1));
        std::string speed_unit = data_str.substr(second_underscore + 1);

        TariffPlan current_tariff = user.selected_tariff;
        TariffSpeedOption selected_option;
        bool found_speed = false;
        for (const auto &opt : current_tariff.speeds)
        {
            if (opt.value == speed_value && opt.unit == speed_unit)
            {
                selected_option = opt;
                found_speed = true;
                break;
            }
        }

        if (!found_speed)
        {
            bot.getApi().sendMessage(chat_id, "Ошибка: выбранная скорость не найдена для тарифа.", nullptr, nullptr, nullptr, "");
            return;
        }

        user.selected_speed_option = selected_option;
        user.final_tariff_string = current_tariff.name + " (" + selected_option.get_full_speed_text() + ")";

        bool needs_tv_box_choice = !current_tariff.tv_box_rental.empty();

        if (needs_tv_box_choice)
        {
            user.state = UserState::CHOOSING_TV;
            db_save_user_state(chat_id, user.state);

            auto tv_keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
            std::vector<TgBot::InlineKeyboardButton::Ptr> row;
            auto yes_btn = std::make_shared<TgBot::InlineKeyboardButton>();
            yes_btn->text = "Да, нужна";
            yes_btn->callbackData = "tv_choice_yes";
            auto no_btn = std::make_shared<TgBot::InlineKeyboardButton>();
            no_btn->text = "Нет, спасибо";
            no_btn->callbackData = "tv_choice_no";
            row.push_back(yes_btn);
            row.push_back(no_btn);
            tv_keyboard->inlineKeyboard.push_back(row);

            std::string text_message = "✅ Выбрали: " + user.final_tariff_string + "\n";
            text_message += "Нужна ли вам ТВ-приставка в аренду за " + current_tariff.tv_box_rental + "?";
            bot.getApi().editMessageText(text_message, chat_id, message_id, "", "Markdown", nullptr, tv_keyboard);
            LOG(LogLevel::INFO, "User (ID: " << chat_id << ") needs to choose TV box for tariff: " << user.final_tariff_string);
        }
        else
        {
            bot.getApi().editMessageText("✅ Выбрали: " + user.final_tariff_string, chat_id, message_id);
            askForName(bot, chat_id);
            LOG(LogLevel::INFO, "User (ID: " << chat_id << ") proceeded to name input for tariff: " << user.final_tariff_string);
        }
        return;
    }

    if (query->data.rfind("tv_choice_", 0) == 0)
    {
        if (user.state != UserState::CHOOSING_TV)
            return;
        bot.getApi().answerCallbackQuery(query->id);
        std::string choice_text = "✅ Выбрали: " + user.final_tariff_string;
        if (query->data == "tv_choice_yes")
        {
            user.needs_tv_box = true;
            choice_text += "\n✅ С ТВ-приставкой";
        }
        else
        {
            user.needs_tv_box = false;
            choice_text += "\n❌ Без ТВ-приставки";
        }
        bot.getApi().editMessageText(choice_text, chat_id, message_id);
        askForName(bot, chat_id);
        return;
    }

    if (query->data.rfind("messenger_", 0) == 0)
    {
        if (user.state != UserState::CHOOSING_MESSENGER)
            return;
        if (query->data == "messenger_telegram")
            user.preferred_messenger = "Telegram";
        else if (query->data == "messenger_whatsapp")
            user.preferred_messenger = "What'sApp";
        else if (query->data == "messenger_max")
            user.preferred_messenger = "MAX";
        bot.getApi().answerCallbackQuery(query->id);
        bot.getApi().editMessageText("Принято: " + user.preferred_messenger, chat_id, message_id);
        askForEmail(bot, chat_id);
        return;
    }

    if (query->data.rfind("faq_", 0) == 0)
    {
        if (user.state != UserState::HELP_SECTION)
            return;
        int index = std::stoi(query->data.substr(4));
        const auto &faq_entries = get_all_faq_entries();
        if (index >= 0 && index < faq_entries.size())
        {
            bot.getApi().answerCallbackQuery(query->id);
            bot.getApi().sendMessage(chat_id, faq_entries[index].answer, nullptr, nullptr, nullptr, "Markdown");
        }
        else
        {
            bot.getApi().answerCallbackQuery(query->id, "Ошибка: вопрос не найден.", true);
        }
        return;
    }

    if (query->data == "back_to_main_menu")
    {
        bot.getApi().answerCallbackQuery(query->id);
        sendMainMenu(bot, chat_id);
        return;
    }

    if (query->data == "back_to_tariff_list")
    {
        if (user.state != UserState::VIEWING_TARIFF_DETAILS && user.state != UserState::CHOOSING_TV)
            return;
        bot.getApi().answerCallbackQuery(query->id);
        sendTariffSelection(bot, chat_id);
        LOG(LogLevel::INFO, "User (ID: " << chat_id << ") returned to main tariff list from details/TV choice.");
        return;
    }
}

void sendMainMenu(TgBot::Bot &bot, int64_t chat_id)
{
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    std::vector<TgBot::KeyboardButton::Ptr> row1, row2, row3;

    // Если есть WebApp URL, добавляем кнопку с WebApp
    if (!config.webapp_url.empty())
    {
        auto btn_webapp = std::make_shared<TgBot::KeyboardButton>();
        btn_webapp->text = "📝 Оставить заявку";
        btn_webapp->webApp = std::make_shared<TgBot::WebAppInfo>();
        btn_webapp->webApp->url = config.webapp_url;
        row1.push_back(btn_webapp);
    }
    else
    {
        auto btn_new_app = std::make_shared<TgBot::KeyboardButton>();
        btn_new_app->text = "📝 Оставить заявку";
        row1.push_back(btn_new_app);
    }

    auto btn_my_apps = std::make_shared<TgBot::KeyboardButton>();
    btn_my_apps->text = "📂 Мои заявки";
    row1.push_back(btn_my_apps);
    keyboard->keyboard.push_back(row1);

    auto check_btn = std::make_shared<TgBot::KeyboardButton>();
    check_btn->text = "🌐 Проверить возможность подключения";
    row2.push_back(check_btn);
    keyboard->keyboard.push_back(row2);

    auto help_btn = std::make_shared<TgBot::KeyboardButton>();
    help_btn->text = "❓ Помощь";
    row3.push_back(help_btn);
    keyboard->keyboard.push_back(row3);

    std::string trade_point_for_admin;
    if (chat_id != config.main_admin_id && db_is_admin_approved(chat_id, trade_point_for_admin))
    {
        std::vector<TgBot::KeyboardButton::Ptr> row_admin;
        auto btn_admin = std::make_shared<TgBot::KeyboardButton>();
        btn_admin->text = "👑 Панель администратора";
        row_admin.push_back(btn_admin);
        keyboard->keyboard.push_back(row_admin);
    }

    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Добро пожаловать в главное меню!", nullptr, nullptr, keyboard);
    user_session_data[chat_id].state = UserState::NONE;
    db_save_user_state(chat_id, UserState::NONE);
}

// Меню после подачи заявки (без кнопки "Оставить заявку" и "Помощь")
void sendPostApplicationMenu(TgBot::Bot &bot, int64_t chat_id)
{
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    std::vector<TgBot::KeyboardButton::Ptr> row1, row2;

    // Мои заявки
    auto btn_my_apps = std::make_shared<TgBot::KeyboardButton>();
    btn_my_apps->text = "📂 Мои заявки";
    row1.push_back(btn_my_apps);

    // Проверить возможность подключения
    auto btn_check = std::make_shared<TgBot::KeyboardButton>();
    btn_check->text = "🌐 Проверить возможность подключения";
    row1.push_back(btn_check);
    keyboard->keyboard.push_back(row1);

    // Связаться с сотрудником
    auto btn_contact = std::make_shared<TgBot::KeyboardButton>();
    btn_contact->text = "📞 Связаться с сотрудником";
    row2.push_back(btn_contact);
    keyboard->keyboard.push_back(row2);

    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Что вы хотите сделать дальше?", nullptr, nullptr, keyboard);
    user_session_data[chat_id].state = UserState::NONE;
    db_save_user_state(chat_id, UserState::NONE);
}

void sendTariffSelection(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::CHOOSING_TARIFF;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto removal_keyboard = std::make_shared<TgBot::ReplyKeyboardRemove>();
    bot.getApi().sendMessage(chat_id, "Загружаю тарифы...", nullptr, nullptr, removal_keyboard);

    auto inline_keyboard = create_tariff_main_buttons();
    bot.getApi().sendMessage(chat_id, "Актуальные тарифы МТС (Москва):", nullptr, nullptr, inline_keyboard);
}

void sendTradePointSelection(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::CHOOSING_FLYER_CODE;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto removal_keyboard = std::make_shared<TgBot::ReplyKeyboardRemove>();
    bot.getApi().sendMessage(chat_id, "Загружаю список точек...", nullptr, nullptr, removal_keyboard);
    std::vector<std::string> codes = get_all_trade_point_codes();
    if (codes.empty())
    {
        bot.getApi().sendMessage(chat_id, "В базе нет ни одной торговой точки. Обратитесь к администратору.", nullptr, nullptr, nullptr, "");
        sendMainMenu(bot, chat_id);
        return;
    }
    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    std::vector<TgBot::InlineKeyboardButton::Ptr> row;
    for (size_t i = 0; i < codes.size(); ++i)
    {
        auto btn = std::make_shared<TgBot::InlineKeyboardButton>();
        btn->text = codes[i];
        btn->callbackData = "flyer_code_" + codes[i];
        row.push_back(btn);
        if (row.size() == 3 || i == codes.size() - 1)
        {
            keyboard->inlineKeyboard.push_back(row);
            row.clear();
        }
    }
    bot.getApi().sendMessage(chat_id, "Выберите код вашей торговой точки из списка:", nullptr, nullptr, keyboard);
}

void askForName(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::ENTERING_NAME;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    sendBackButtonKeyboard(bot, chat_id, "Для оформления заявки введите ваше имя:");
}

void askForPhone(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::ENTERING_PHONE;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    auto contact_btn = std::make_shared<TgBot::KeyboardButton>();
    contact_btn->text = "📱 Отправить мой контакт";
    contact_btn->requestContact = true;
    auto back_btn = std::make_shared<TgBot::KeyboardButton>();
    back_btn->text = "⬅️ Назад";
    keyboard->keyboard.push_back({contact_btn});
    keyboard->keyboard.push_back({back_btn});
    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Спасибо! Теперь введите номер телефона для связи (10 цифр).\nПример: 912 345 67 89", nullptr, nullptr, keyboard);
}

void askForMessenger(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::CHOOSING_MESSENGER;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto messenger_keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    std::vector<TgBot::InlineKeyboardButton::Ptr> messenger_row;
    auto tg_btn = std::make_shared<TgBot::InlineKeyboardButton>();
    tg_btn->text = "Телеграм";
    tg_btn->callbackData = "messenger_telegram";
    messenger_row.push_back(tg_btn);
    auto wa_btn = std::make_shared<TgBot::InlineKeyboardButton>();
    wa_btn->text = "What'sApp";
    wa_btn->callbackData = "messenger_whatsapp";
    messenger_row.push_back(wa_btn);
    auto max_btn = std::make_shared<TgBot::InlineKeyboardButton>();
    max_btn->text = "MAX";
    max_btn->callbackData = "messenger_max";
    messenger_row.push_back(max_btn);
    messenger_keyboard->inlineKeyboard.push_back(messenger_row);
    auto back_keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    auto back_btn = std::make_shared<TgBot::KeyboardButton>();
    back_btn->text = "⬅️ Назад";
    back_keyboard->keyboard.push_back({back_btn});
    back_keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Принято! Куда вам будет удобнее написать?", nullptr, nullptr, back_keyboard);
    bot.getApi().sendMessage(chat_id, "Выберите мессенджер:", nullptr, nullptr, messenger_keyboard);
}

void askForEmail(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::ENTERING_EMAIL;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    auto btn = std::make_shared<TgBot::KeyboardButton>();
    btn->text = "⬅️ Назад";
    keyboard->keyboard.push_back({btn});
    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Отлично! Теперь введите вашу электронную почту:", nullptr, nullptr, keyboard);
}

void askForCity(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::ENTERING_CITY;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    auto btn = std::make_shared<TgBot::KeyboardButton>();
    btn->text = "⬅️ Назад";
    keyboard->keyboard.push_back({btn});
    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Теперь начнем ввод адреса. Введите ваш город:", nullptr, nullptr, keyboard);
}

void askForStreet(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::ENTERING_STREET;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    auto btn = std::make_shared<TgBot::KeyboardButton>();
    btn->text = "⬅️ Назад";
    keyboard->keyboard.push_back({btn});
    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Принято! Введите улицу:", nullptr, nullptr, keyboard);
}

void askForHouse(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::ENTERING_HOUSE;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    auto btn = std::make_shared<TgBot::KeyboardButton>();
    btn->text = "⬅️ Назад";
    keyboard->keyboard.push_back({btn});
    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Введите номер дома:", nullptr, nullptr, keyboard);
}

void askForHouseBody(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::ENTERING_HOUSE_BODY;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    std::vector<TgBot::KeyboardButton::Ptr> row;
    auto skip_btn = std::make_shared<TgBot::KeyboardButton>();
    skip_btn->text = "Пропустить";
    auto back_btn = std::make_shared<TgBot::KeyboardButton>();
    back_btn->text = "⬅️ Назад";
    row.push_back(skip_btn);
    row.push_back(back_btn);
    keyboard->keyboard.push_back(row);
    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Введите номер корпуса (если есть):", nullptr, nullptr, keyboard);
}

void askForApartment(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::ENTERING_APARTMENT;
    db_save_user_state(chat_id, user_session_data[chat_id].state);
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    std::vector<TgBot::KeyboardButton::Ptr> row;
    auto skip_btn = std::make_shared<TgBot::KeyboardButton>();
    skip_btn->text = "Пропустить";
    auto back_btn = std::make_shared<TgBot::KeyboardButton>();
    back_btn->text = "⬅️ Назад";
    row.push_back(skip_btn);
    row.push_back(back_btn);
    keyboard->keyboard.push_back(row);
    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, "Введите номер квартиры (если есть):", nullptr, nullptr, keyboard);
}

void sendHelpMenu(TgBot::Bot &bot, int64_t chat_id)
{
    user_session_data[chat_id].state = UserState::HELP_SECTION;
    db_save_user_state(chat_id, UserState::HELP_SECTION);

    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    const auto &faq_entries = get_all_faq_entries();

    for (size_t i = 0; i < faq_entries.size(); ++i)
    {
        auto btn = std::make_shared<TgBot::InlineKeyboardButton>();
        btn->text = faq_entries[i].question;
        btn->callbackData = "faq_" + std::to_string(i);
        keyboard->inlineKeyboard.push_back({btn});
    }

    auto back_btn = std::make_shared<TgBot::InlineKeyboardButton>();
    back_btn->text = "⬅️ Назад в главное меню";
    back_btn->callbackData = "back_to_main_menu";
    keyboard->inlineKeyboard.push_back({back_btn});

    bot.getApi().sendMessage(chat_id, "Чем я могу вам помочь? Выберите вопрос из списка:", nullptr, nullptr, keyboard);
}

void sendBackButtonKeyboard(TgBot::Bot &bot, int64_t chat_id, const std::string &text)
{
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    auto back_btn = std::make_shared<TgBot::KeyboardButton>();
    back_btn->text = "⬅️ Назад";
    keyboard->keyboard.push_back({back_btn});
    keyboard->resizeKeyboard = true;
    bot.getApi().sendMessage(chat_id, text, nullptr, nullptr, keyboard);
}
