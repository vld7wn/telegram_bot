#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>

// Forward declarations
struct UserData;
struct ApplicationDataForReport;
enum class UserState;
enum class AdminWorkMode;
enum class ApplicationStatus;
enum class ChatStatus;

// Структура для хранения данных заявки на админство
struct AdminRequestData {
    int64_t user_id;
    std::string name;
    std::string trade_point;
};

// Новая структура для сообщения в чате
struct ChatMessage {
    std::string sender; // "admin", "client"
    std::string text;
    std::string timestamp;
};

// Определяем путь к базе данных
#define DB_PATH "db/bot_data.db"

void db_init();
void db_close();

// Функции для управления статусом бота
void db_init_bot_settings();
bool db_get_bot_status();
void db_set_bot_status(bool active_status);

// Функции для работы с заявками
void db_add_application(int64_t user_id, const UserData& data, const std::string& full_address, int total_monthly);
std::string db_get_my_apps(int64_t user_id);
std::string db_get_apps_by_trade_point(const std::string& trade_point_code);
std::vector<ApplicationDataForReport> db_get_apps_data_for_report(const std::string& trade_point_code);
void db_update_application_status(long long application_id, ApplicationStatus status);

// Функции для управления администраторами
void db_add_admin_request(int64_t user_id, const std::string& name, const std::string& trade_point);
void db_approve_admin(int64_t user_id);
bool db_is_admin_approved(int64_t user_id, std::string& trade_point);
std::vector<AdminRequestData> db_get_pending_admin_requests();
void db_decline_admin_request(int64_t user_id);
std::vector<int64_t> db_get_admin_ids_by_trade_point(const std::string& trade_point);
std::vector<AdminRequestData> db_get_all_admins();
void db_add_admin_manual(int64_t user_id, const std::string& name, const std::string& trade_point);
void db_delete_admin(int64_t user_id);
bool db_admin_exists(int64_t user_id);
int64_t db_get_chat_admin(long long application_id); // <-- Добавлено

// Функции для управления сессиями пользователей
void db_save_user_state(int64_t user_id, UserState state);
void db_load_user_states(std::map<int64_t, UserData>& session_map);
void db_save_admin_work_mode(int64_t user_id, AdminWorkMode mode);
void db_load_admin_work_modes(std::map<int64_t, AdminWorkMode>& mode_map);
void db_delete_session(int64_t user_id);
AdminWorkMode db_get_admin_work_mode(int64_t user_id);

// Вспомогательные функции
std::string statusToString(ApplicationStatus status);
std::string chatStatusToString(ChatStatus status);

// Функции для работы с чатом
void db_update_chat_status(long long application_id, ChatStatus status, int64_t admin_id = 0, const std::string& postponed_until = "");
void db_add_chat_message(long long application_id, const ChatMessage& message);
std::vector<ChatMessage> db_get_chat_history(long long application_id);