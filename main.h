#pragma once
#include <tgbot/tgbot.h>
#include <map>
#include <string>

// Включаем наши централизованные определения
#include "user_data_types.h"
#include "super_admin.h"
#include "application_status.h"
#include "message_to_client.h"

// Объявления глобальных переменных (ссылки на данные в SessionManager)
extern std::map<int64_t, std::string> &admin_otps;
extern std::map<int64_t, UserData> &user_session_data;
extern std::map<int64_t, AdminWorkMode> &admin_work_mode;