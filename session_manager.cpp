#include "session_manager.h"
#include "user_data_types.h"
#include "super_admin.h"

SessionManager &SessionManager::instance()
{
    static SessionManager instance;
    return instance;
}

// ================== User Data ==================

UserData &SessionManager::getUserData(int64_t user_id)
{
    std::lock_guard<std::mutex> lock(mtx_);
    return user_sessions_[user_id];
}

bool SessionManager::hasUserData(int64_t user_id) const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return user_sessions_.count(user_id) > 0;
}

void SessionManager::removeUserData(int64_t user_id)
{
    std::lock_guard<std::mutex> lock(mtx_);
    user_sessions_.erase(user_id);
}

// ================== Admin Mode ==================

AdminWorkMode SessionManager::getAdminMode(int64_t user_id) const
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = admin_modes_.find(user_id);
    if (it != admin_modes_.end())
    {
        return it->second;
    }
    return AdminWorkMode::UNKNOWN;
}

void SessionManager::setAdminMode(int64_t user_id, AdminWorkMode mode)
{
    std::lock_guard<std::mutex> lock(mtx_);
    admin_modes_[user_id] = mode;
}

bool SessionManager::hasAdminMode(int64_t user_id) const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return admin_modes_.count(user_id) > 0;
}

// ================== OTP ==================

std::string SessionManager::getOtp(int64_t user_id) const
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = admin_otps_.find(user_id);
    if (it != admin_otps_.end())
    {
        return it->second;
    }
    return "";
}

void SessionManager::setOtp(int64_t user_id, const std::string &otp)
{
    std::lock_guard<std::mutex> lock(mtx_);
    admin_otps_[user_id] = otp;
}

bool SessionManager::hasOtp(int64_t user_id) const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return admin_otps_.count(user_id) > 0;
}

void SessionManager::removeOtp(int64_t user_id)
{
    std::lock_guard<std::mutex> lock(mtx_);
    admin_otps_.erase(user_id);
}

// ================== Bulk Access (для загрузки из БД) ==================

std::map<int64_t, UserData> &SessionManager::getAllUserSessions()
{
    return user_sessions_;
}

std::map<int64_t, AdminWorkMode> &SessionManager::getAllAdminModes()
{
    return admin_modes_;
}
