#pragma once
#include <map>
#include <string>
#include <cstdint>
#include <mutex>

// Forward declarations
struct UserData;
enum class AdminWorkMode;

/**
 * SessionManager - Singleton для управления сессиями пользователей
 * Инкапсулирует глобальные переменные: user_session_data, admin_work_mode, admin_otps
 */
class SessionManager
{
public:
    static SessionManager &instance();

    // Управление данными пользователей
    UserData &getUserData(int64_t user_id);
    bool hasUserData(int64_t user_id) const;
    void removeUserData(int64_t user_id);

    // Управление режимами работы админов
    AdminWorkMode getAdminMode(int64_t user_id) const;
    void setAdminMode(int64_t user_id, AdminWorkMode mode);
    bool hasAdminMode(int64_t user_id) const;

    // Управление OTP для админов
    std::string getOtp(int64_t user_id) const;
    void setOtp(int64_t user_id, const std::string &otp);
    bool hasOtp(int64_t user_id) const;
    void removeOtp(int64_t user_id);

    // Загрузка/сохранение состояний (для интеграции с БД)
    std::map<int64_t, UserData> &getAllUserSessions();
    std::map<int64_t, AdminWorkMode> &getAllAdminModes();

private:
    SessionManager() = default;
    ~SessionManager() = default;

    SessionManager(const SessionManager &) = delete;
    SessionManager &operator=(const SessionManager &) = delete;

    std::map<int64_t, UserData> user_sessions_;
    std::map<int64_t, AdminWorkMode> admin_modes_;
    std::map<int64_t, std::string> admin_otps_;

    mutable std::mutex mtx_;
};
