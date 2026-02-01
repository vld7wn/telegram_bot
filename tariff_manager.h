#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <tgbot/tgbot.h>
#include <sstream>

struct TariffSpeedOption
{
    std::string value;
    std::string unit;
    std::string price;
    std::string promo_price_duration_months;
    std::string full_price;

    std::string get_full_speed_text() const
    {
        return value + " " + unit;
    }
};

struct TariffPlan
{
    std::string id;
    std::string name;

    bool mobile_connection_included;
    std::string mobile_internet_gb;
    std::string mobile_minutes;
    std::string mobile_sms;

    bool tv_kion;
    std::string tv_channels;
    std::string router_rental;
    std::string tv_box_rental;
    std::string connection_fee;
    std::vector<TariffSpeedOption> speeds;
    bool internet_unlimited;           // For home internet
    std::string internet_limit_gb;     // For home internet
    std::vector<std::string> services; // Dynamic list of included services
    std::string extra_details;         // Additional field for home internet

    std::string get_tariff_description() const;
};

extern std::vector<TariffPlan> tariff_plans;

void load_tariff_plans(const std::string &filename);
void save_tariff_plans(const std::string &filename); // New
void update_tariff_plan(const TariffPlan &tariff);   // New
void apply_discount_to_tariffs(const std::vector<std::string> &ids, double percent);

std::vector<std::string> get_all_tariff_main_ids();
TariffPlan get_tariff_by_id(const std::string &id);
std::vector<std::string> get_all_unique_speeds();

TgBot::InlineKeyboardMarkup::Ptr create_tariff_main_buttons();
TgBot::InlineKeyboardMarkup::Ptr create_tariff_speed_buttons(const std::string &tariff_id);