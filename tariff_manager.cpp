#include "tariff_manager.h"
#include "logger.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <set>

std::vector<TariffPlan> tariff_plans;

void load_tariff_plans(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG(LogLevel::L_ERROR, "Failed to open tariff plans file: " << filename);
        return;
    }
    LOG(LogLevel::INFO, "Successfully opened tariff plans file: " << filename); // <-- ДОБАВЛЕНО
    try {
        nlohmann::json root = nlohmann::json::parse(file);
        LOG(LogLevel::INFO, "Successfully parsed tariff plans JSON.");
        if (!root.is_array()) {
            LOG(LogLevel::L_ERROR, "Root of tariff plans JSON is not an array.");
            return;
        }

        tariff_plans.clear();

        for (const auto& tariff_json : root) {
            TariffPlan tariff;
            tariff.id = tariff_json.at("id").get<std::string>();
            tariff.name = tariff_json.at("name").get<std::string>();

            // Parsing mobile connection details
            tariff.mobile_connection_included = tariff_json.at("mobile_connection_included").get<bool>();
            if (tariff.mobile_connection_included) {
                tariff.mobile_internet_gb = tariff_json.value("mobile_internet_gb", "");
                tariff.mobile_minutes = tariff_json.value("mobile_minutes", "");
                tariff.mobile_sms = tariff_json.value("mobile_sms", "");
            } else {
                tariff.mobile_internet_gb = "";
                tariff.mobile_minutes = "";
                tariff.mobile_sms = "";
            }

            tariff.tv_kion = tariff_json.at("tv_kion").get<bool>();
            tariff.tv_channels = tariff_json.value("tv_channels", ""); // Use value() for optional or empty
            tariff.router_rental = tariff_json.at("router_rental").get<std::string>();
            tariff.tv_box_rental = tariff_json.value("tv_box_rental", ""); // Use value() for optional or empty
            tariff.connection_fee = tariff_json.at("connection_fee").get<std::string>();
            tariff.internet_unlimited = tariff_json.at("internet_unlimited").get<bool>();
            tariff.internet_limit_gb = tariff_json.value("internet_limit_gb", ""); // Use value() for optional or empty

            for (const auto& speed_json : tariff_json.at("speeds")) {
                TariffSpeedOption speed_option;
                speed_option.value = speed_json.at("value").get<std::string>();
                speed_option.unit = speed_json.at("unit").get<std::string>();
                speed_option.price = speed_json.at("price").get<std::string>();
                speed_option.promo_price_duration_months = speed_json.value("promo_price_duration_months", "");
                speed_option.full_price = speed_json.value("full_price", "");
                tariff.speeds.push_back(speed_option);
            }
            LOG(LogLevel::INFO, "Parsed tariff: " << tariff.name << " with ID: " << tariff.id);
            tariff_plans.push_back(tariff);
        }
        LOG(LogLevel::INFO, "Tariff plans loaded successfully from " << filename << ". Total tariffs: " << tariff_plans.size());
    } catch (const nlohmann::json::exception& e) {
        LOG(LogLevel::L_ERROR, "Failed to parse tariff plans JSON or access data: " << e.what());
    } catch (const std::exception& e) {
        LOG(LogLevel::L_ERROR, "An unexpected error occurred during tariff loading: " << e.what());
    }
}

std::vector<std::string> get_all_tariff_main_ids() {
    std::vector<std::string> ids;
    for (const auto& tariff : tariff_plans) {
        ids.push_back(tariff.id);
    }
    return ids;
}

TariffPlan get_tariff_by_id(const std::string& id) {
    for (const auto& tariff : tariff_plans) {
        if (tariff.id == id) {
            return tariff;
        }
    }
    LOG(LogLevel::L_WARNING, "Tariff not found by ID: " << id);
    return TariffPlan{};
}

std::vector<std::string> get_all_unique_speeds() {
    std::set<std::string> unique_speeds;
    for (const auto& tariff : tariff_plans) {
        for (const auto& speed_option : tariff.speeds) {
            unique_speeds.insert(speed_option.get_full_speed_text());
        }
    }
    std::vector<std::string> speeds(unique_speeds.begin(), unique_speeds.end());
    std::sort(speeds.begin(), speeds.end(), [](const std::string& a, const std::string& b) {
        int val_a = std::stoi(a.substr(0, a.find(' ')));
        int val_b = std::stoi(b.substr(0, b.find(' ')));
        return val_a < val_b;
    });
    return speeds;
}

std::string TariffPlan::get_tariff_description() const {
    std::stringstream text;
    text << "*" << name << "*\n\n";

    if (mobile_connection_included) {
        text << "• *Моб. связь:* Включена в тариф";
        bool has_details = false;
        if (!mobile_internet_gb.empty()) {
            if (!has_details) text << " ("; else text << ", ";
            text << mobile_internet_gb << " ГБ";
            has_details = true;
        }
        if (!mobile_minutes.empty()) {
            if (!has_details) text << " ("; else text << ", ";
            text << mobile_minutes << " мин";
            has_details = true;
        }
        if (!mobile_sms.empty()) {
            if (!has_details) text << " ("; else text << ", ";
            text << mobile_sms << " СМС";
            has_details = true;
        }
        if (has_details) text << ")";
        text << "\n";
    } else {
        text << "• *Моб. связь:* Не включена\n";
    }

    // Home internet details
    if (internet_unlimited) {
        text << "• *Интернет (дом):* Безлимитный\n"; // Added (дом) for clarity
    } else {
        if (!internet_limit_gb.empty()) {
            text << "• *Интернет (дом):* " << internet_limit_gb << " ГБ/мес\n"; // Added (дом) for clarity
        } else {
            text << "• *Интернет (дом):* Лимитированный (объем не указан)\n";
        }
    }

    if (tv_kion) {
        text << "• *ТВ:* Онлайн-кинотеатр KION\n";
    } else if (!tv_channels.empty()) {
        text << "• *ТВ:* " << tv_channels << "\n";
    }

    if (!router_rental.empty()) {
        text << "• *Аренда роутера:* " << router_rental << "\n";
    }
    if (!tv_box_rental.empty()) {
        text << "• *Аренда ТВ-приставки:* " << tv_box_rental << "\n";
    }

    text << "\n*Стоимость в месяц:*\n";
    for (const auto& speed_option : speeds) {
        text << "  - " << speed_option.get_full_speed_text() << ": *" << speed_option.price << " ₽*";
        if (!speed_option.promo_price_duration_months.empty() && !speed_option.full_price.empty()) {
            text << " (" << speed_option.promo_price_duration_months << " мес, далее " << speed_option.full_price << " ₽)";
        }
        text << "\n";
    }

    text << "\n*Подключение:* " << connection_fee << " (единоразово)";

    return text.str();
}

TgBot::InlineKeyboardMarkup::Ptr create_tariff_main_buttons() {
    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    std::vector<TgBot::InlineKeyboardButton::Ptr> row;
    int buttons_per_row = 1;

    if (tariff_plans.empty()) {
        LOG(LogLevel::L_WARNING, "Attempted to create tariff buttons but tariff_plans vector is empty.");
        return keyboard; // Возвращаем пустую клавиатуру
    }

    for (const auto& tariff : tariff_plans) {
        auto btn = std::make_shared<TgBot::InlineKeyboardButton>();
        btn->text = tariff.name;
        btn->callbackData = "show_tariff_detail_" + tariff.id;
        row.push_back(btn);

        if (row.size() == buttons_per_row) {
            keyboard->inlineKeyboard.push_back(row);
            row.clear();
        }
    }
    if (!row.empty()) {
        keyboard->inlineKeyboard.push_back(row);
    }
    LOG(LogLevel::INFO, "DEBUG: create_tariff_main_buttons finished. Total rows: " << keyboard->inlineKeyboard.size()); // <-- ДОБАВЛЕНО
    return keyboard;
}

TgBot::InlineKeyboardMarkup::Ptr create_tariff_speed_buttons(const std::string& tariff_id) {
    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    TariffPlan tariff = get_tariff_by_id(tariff_id);

    if (tariff.id.empty()) {
        return nullptr;
    }

    std::vector<TgBot::InlineKeyboardButton::Ptr> row;
    for (const auto& speed_option : tariff.speeds) {
        auto btn = std::make_shared<TgBot::InlineKeyboardButton>();
        btn->text = "✅ Выбрать " + speed_option.get_full_speed_text();
        btn->callbackData = "select_speed_" + tariff_id + "_" + speed_option.value + "_" + speed_option.unit;
        row.push_back(btn);
        keyboard->inlineKeyboard.push_back(row);
        row.clear();
    }

    std::vector<TgBot::InlineKeyboardButton::Ptr> back_row;
    auto back_btn = std::make_shared<TgBot::InlineKeyboardButton>();
    back_btn->text = "⬅️ Назад к тарифам";
    back_btn->callbackData = "back_to_tariff_list";
    back_row.push_back(back_btn);
    keyboard->inlineKeyboard.push_back(back_row);

    return keyboard;
}