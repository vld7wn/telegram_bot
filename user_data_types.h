#pragma once
#include <string>
#include <vector>
#include "tariff_manager.h"
#include "super_admin.h"
#include "application_status.h"

// Перечисление для статусов чата
enum class ChatStatus
{
    New,
    InProgress,
    Postponed,
    Completed
};

// Перечисление состояний пользователя, используется для навигации по меню и сценариям.
enum class UserState
{
    NONE,
    HELP_SECTION,
    AWAITING_ADDRESS_FOR_CHECK,
    AWAITING_ADMIN_TRADE_POINT_CHOICE,
    AWAITING_ADMIN_NAME,
    AWAITING_APPROVAL,
    AWAITING_ADMIN_PASSWORD,
    ADMIN_PANEL,
    CHOOSING_FLYER_CODE,
    CHOOSING_TARIFF,
    VIEWING_TARIFF_DETAILS,
    CHOOSING_TV,
    ENTERING_NAME,
    ENTERING_PHONE,
    CHOOSING_MESSENGER,
    ENTERING_EMAIL,
    ENTERING_CITY,
    ENTERING_STREET,
    ENTERING_HOUSE,
    ENTERING_HOUSE_BODY,
    ENTERING_APARTMENT,
    AWAITING_FIELD_TO_EDIT,
    ADMIN_REPLYING_TO_USER // <--- ДОБАВЛЕНО
};

// Структура для хранения данных о заявке для отчета.
struct ApplicationDataForReport
{
    long long id;
    long long user_id;
    std::string tariff;
    std::string name;
    std::string price;
    std::string phone;
    std::string email;
    std::string address;
    std::string timestamp;
    std::string chat_status;
};

// Структура для хранения данных пользователя в сессии.
struct UserData
{
    UserState state = UserState::NONE;
    std::string admin_trade_point;
    std::string admin_name;
    int64_t new_admin_id_to_approve = 0;
    std::string temp_trade_point_code;
    std::string flyer_code;
    std::string office_address;

    TariffPlan selected_tariff;
    TariffSpeedOption selected_speed_option;

    bool needs_tv_box = false;
    std::string final_tariff_string;
    int base_price = 0;

    std::string name;
    std::string phone;
    std::string preferred_messenger;
    std::string email;
    std::string city;
    std::string street;
    std::string house;
    std::string house_body;
    std::string apartment;
    bool is_editing = false;
    int64_t reply_to_user_id = 0;
    long long current_application_id = 0;
};

extern std::map<int64_t, std::string> &admin_otps;
extern std::map<int64_t, UserData> &user_session_data;
extern std::map<int64_t, AdminWorkMode> &admin_work_mode;
