#pragma once
#include "state_handler.h"
#include "user_data_types.h"
#include "database.h"
#include "logger.h"
#include <sstream>

// Forward declarations
void sendMainMenu(TgBot::Bot &bot, int64_t chat_id);
void askForName(TgBot::Bot &bot, int64_t chat_id);
void askForPhone(TgBot::Bot &bot, int64_t chat_id);
void askForMessenger(TgBot::Bot &bot, int64_t chat_id);
void askForEmail(TgBot::Bot &bot, int64_t chat_id);
void askForCity(TgBot::Bot &bot, int64_t chat_id);
void askForStreet(TgBot::Bot &bot, int64_t chat_id);
void askForHouse(TgBot::Bot &bot, int64_t chat_id);
void askForHouseBody(TgBot::Bot &bot, int64_t chat_id);
void askForApartment(TgBot::Bot &bot, int64_t chat_id);
void notifyAdminsOfNewApplication(TgBot::Bot &bot, const std::string &trade_point_code, const std::string &notification_text);
bool isNotEmpty(const std::string &str);
bool isValidPhone(const std::string &phone);
bool isValidEmail(const std::string &email);

/**
 * Базовый класс для обработчиков состояний ввода данных
 */
class BaseInputHandler : public IStateHandler
{
public:
    bool handleCallback(TgBot::Bot &bot, TgBot::CallbackQuery::Ptr query, UserData &user) override
    {
        // Большинство состояний ввода не обрабатывают callbacks
        return false;
    }

    bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) override
    {
        // По умолчанию возвращаемся в главное меню
        sendMainMenu(bot, chat_id);
        return true;
    }
};

/**
 * Обработчик ввода имени
 */
class EnterNameHandler : public BaseInputHandler
{
public:
    bool handleMessage(TgBot::Bot &bot, TgBot::Message::Ptr message, UserData &user) override
    {
        int64_t chat_id = message->chat->id;

        if (isNotEmpty(message->text))
        {
            user.name = message->text;
            askForPhone(bot, chat_id);
            return true;
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Имя не может быть пустым.");
            return true;
        }
    }

    bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) override
    {
        // Возврат к выбору тарифа - обработается в основном коде
        return false;
    }
};

/**
 * Обработчик ввода телефона
 */
class EnterPhoneHandler : public BaseInputHandler
{
public:
    bool handleMessage(TgBot::Bot &bot, TgBot::Message::Ptr message, UserData &user) override
    {
        int64_t chat_id = message->chat->id;
        std::string phone_number = (message->contact != nullptr) ? message->contact->phoneNumber : message->text;

        if (isValidPhone(phone_number))
        {
            user.phone = phone_number;
            askForMessenger(bot, chat_id);
            return true;
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Неверный формат. Введите 10 цифр или отправьте контакт.\nПример: 912 345 67 89");
            return true;
        }
    }

    bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) override
    {
        askForName(bot, chat_id);
        return true;
    }
};

/**
 * Обработчик ввода email
 */
class EnterEmailHandler : public BaseInputHandler
{
public:
    bool handleMessage(TgBot::Bot &bot, TgBot::Message::Ptr message, UserData &user) override
    {
        int64_t chat_id = message->chat->id;

        if (isValidEmail(message->text))
        {
            user.email = message->text;
            askForCity(bot, chat_id);
            return true;
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Неверный формат почты (например, user@example.com).");
            return true;
        }
    }

    bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) override
    {
        askForMessenger(bot, chat_id);
        return true;
    }
};

/**
 * Обработчик ввода города
 */
class EnterCityHandler : public BaseInputHandler
{
public:
    bool handleMessage(TgBot::Bot &bot, TgBot::Message::Ptr message, UserData &user) override
    {
        int64_t chat_id = message->chat->id;

        if (isNotEmpty(message->text))
        {
            user.city = message->text;
            askForStreet(bot, chat_id);
            return true;
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Название города не может быть пустым.");
            return true;
        }
    }

    bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) override
    {
        askForEmail(bot, chat_id);
        return true;
    }
};

/**
 * Обработчик ввода улицы
 */
class EnterStreetHandler : public BaseInputHandler
{
public:
    bool handleMessage(TgBot::Bot &bot, TgBot::Message::Ptr message, UserData &user) override
    {
        int64_t chat_id = message->chat->id;

        if (isNotEmpty(message->text))
        {
            user.street = message->text;
            askForHouse(bot, chat_id);
            return true;
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Название улицы не может быть пустым.");
            return true;
        }
    }

    bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) override
    {
        askForCity(bot, chat_id);
        return true;
    }
};

/**
 * Обработчик ввода номера дома
 */
class EnterHouseHandler : public BaseInputHandler
{
public:
    bool handleMessage(TgBot::Bot &bot, TgBot::Message::Ptr message, UserData &user) override
    {
        int64_t chat_id = message->chat->id;

        if (isNotEmpty(message->text))
        {
            user.house = message->text;
            askForHouseBody(bot, chat_id);
            return true;
        }
        else
        {
            bot.getApi().sendMessage(chat_id, "Номер дома не может быть пустым.");
            return true;
        }
    }

    bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) override
    {
        askForStreet(bot, chat_id);
        return true;
    }
};

/**
 * Обработчик ввода корпуса
 */
class EnterHouseBodyHandler : public BaseInputHandler
{
public:
    bool handleMessage(TgBot::Bot &bot, TgBot::Message::Ptr message, UserData &user) override
    {
        int64_t chat_id = message->chat->id;

        if (message->text != "Пропустить")
        {
            user.house_body = message->text;
        }
        else
        {
            user.house_body = "";
        }
        askForApartment(bot, chat_id);
        return true;
    }

    bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) override
    {
        askForHouse(bot, chat_id);
        return true;
    }
};

/**
 * Обработчик ввода квартиры - финальный шаг с формированием заявки
 */
class EnterApartmentHandler : public BaseInputHandler
{
public:
    bool handleMessage(TgBot::Bot &bot, TgBot::Message::Ptr message, UserData &user) override
    {
        int64_t chat_id = message->chat->id;

        if (message->text != "Пропустить")
        {
            user.apartment = message->text;
        }
        else
        {
            user.apartment = "не указана";
        }

        // Расчёт стоимости
        int total_monthly = std::stoi(user.selected_speed_option.price);
        std::string rent_details = calculateRentDetails(user, total_monthly);
        int connection_fee = parseConnectionFee(user);

        // Формирование счёта для клиента
        std::string client_invoice = formatClientInvoice(user, total_monthly, rent_details, connection_fee);
        bot.getApi().sendMessage(chat_id, client_invoice, false, 0, nullptr, "Markdown");

        // Сохранение в БД
        std::string full_address = formatFullAddress(user);
        db_add_application(chat_id, user, full_address, total_monthly);

        // Уведомление админов
        std::stringstream notification_text;
        notification_text << "Имя: " << user.name << "\n"
                          << "Телефон: " << user.phone << "\n"
                          << "Тариф: " << user.final_tariff_string;
        notifyAdminsOfNewApplication(bot, user.flyer_code, notification_text.str());

        // Финальное сообщение клиенту
        std::string final_message = "Ваша заявка принята. Скоро с вами свяжутся для уточнения деталей.\n\n"
                                    "Для подключения интернета подойдите по адресу:\n*" +
                                    user.office_address + "*\n\n"
                                                          "**Не забудьте взять с собой паспорт!**";
        bot.getApi().sendMessage(chat_id, final_message, false, 0, nullptr, "Markdown");
        sendMainMenu(bot, chat_id);

        return true;
    }

    bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) override
    {
        askForHouseBody(bot, chat_id);
        return true;
    }

private:
    std::string calculateRentDetails(UserData &user, int &total_monthly)
    {
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
                LOG(LogLevel::L_ERROR, "Failed to parse router_rental price: " << user.selected_tariff.router_rental);
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
                LOG(LogLevel::L_ERROR, "Failed to parse tv_box_rental price: " << user.selected_tariff.tv_box_rental);
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

        return rent_details;
    }

    int parseConnectionFee(const UserData &user)
    {
        int connection_fee = 0;
        if (!user.selected_tariff.connection_fee.empty())
        {
            try
            {
                connection_fee = std::stoi(user.selected_tariff.connection_fee.substr(0, user.selected_tariff.connection_fee.find(' ')));
            }
            catch (const std::exception &e)
            {
                LOG(LogLevel::L_ERROR, "Failed to parse connection_fee price: " << user.selected_tariff.connection_fee);
            }
        }
        return connection_fee;
    }

    std::string formatClientInvoice(const UserData &user, int total_monthly, const std::string &rent_details, int connection_fee)
    {
        std::stringstream ss;
        ss << "*Ваша заявка сформирована:*\n\n"
           << "👤 *Имя:* " << user.name << "\n"
           << "📡 *Тариф:* " << user.final_tariff_string << "\n"
           << "🏠 *Адрес:* г. " << user.city << ", ул. " << user.street << ", д. " << user.house;

        if (!user.house_body.empty())
            ss << ", корп. " << user.house_body;
        if (user.apartment != "не указана")
            ss << ", кв. " << user.apartment;

        ss << "\n\n*Расчет стоимости:*\n"
           << "Ежемесячная плата по тарифу: *" << user.selected_speed_option.price << " ₽*\n";

        if (!user.selected_speed_option.promo_price_duration_months.empty())
        {
            ss << "(акция: " << user.selected_speed_option.promo_price_duration_months
               << " мес, далее " << user.selected_speed_option.full_price << " ₽)\n";
        }

        ss << "Аренда оборудования: *" << rent_details << "*\n"
           << "*Итого к оплате ежемесячно: " << total_monthly << " ₽*\n\n"
           << "*Единоразовый платеж за подключение: " << connection_fee << " ₽*";

        return ss.str();
    }

    std::string formatFullAddress(const UserData &user)
    {
        std::string full_address = "г. " + user.city + ", ул. " + user.street + ", д. " + user.house;
        if (!user.house_body.empty())
        {
            full_address += ", корп. " + user.house_body;
        }
        if (user.apartment != "не указана")
        {
            full_address += ", кв. " + user.apartment;
        }
        return full_address;
    }
};
