#include "database.h"
#include "user_data_types.h"
#include "application_status.h"
#include "main.h"
#include "config.h"
#include "logger.h"
#include <sqlite3.h>
#include <cstdio>
#include <sstream>

sqlite3 *db_main;

// Callback-функция для вывода заявок пользователя.
static int db_my_apps_callback(void *data, int argc, char **argv, char **azColName)
{
    auto *result = static_cast<std::string *>(data);
    *result += "---------------------------------\n";
    *result += "*Заявка от " + std::string(argv[4] ? argv[4] : "не указана") + "*\n";
    *result += "📡 *Тариф:* " + std::string(argv[0] ? argv[0] : "") + "\n";
    *result += "💰 *Стоимость:* " + std::string(argv[1] ? argv[1] : "") + "\n";
    *result += "🏠 *Адрес:* " + std::string(argv[2] ? argv[2] : "") + "\n";
    *result += "📈 *Статус:* " + std::string(argv[3] ? argv[3] : "Новая") + "\n";
    return 0;
}

// Callback-функция для вывода заявок администратору.
static int db_admin_callback(void *data, int argc, char **argv, char **azColName)
{
    auto *result = static_cast<std::string *>(data);
    std::string phone = argv[5] ? argv[5] : "";
    *result += "---------------------------------\n";
    *result += "*Заявка №" + std::string(argv[0] ? argv[0] : "") + " от " + std::string(argv[9] ? argv[9] : "") + "*\n";
    *result += "👤 *Клиент:* " + std::string(argv[3] ? argv[3] : "") + " (ID: " + std::string(argv[1] ? argv[1] : "") + ")\n";
    *result += "📞 *Телефон:* " + phone + "\n";
    *result += "📡 *Тариф:* " + std::string(argv[2] ? argv[2] : "") + "\n";
    *result += "💰 *Стоимость:* " + std::string(argv[4] ? argv[4] : "") + "\n";
    *result += "✉️ *Почта:* " + std::string(argv[6] ? argv[6] : "") + "\n";
    *result += "🏠 *Адрес:* " + std::string(argv[7] ? argv[7] : "") + "\n";
    *result += "📈 *Статус:* " + std::string(argv[8] ? argv[8] : "Новая") + "\n";
    return 0;
}

// Callback-функция для получения истории чата.
static int db_chat_history_callback(void *data, int argc, char **argv, char **azColName)
{
    auto *history = static_cast<std::vector<ChatMessage> *>(data);
    ChatMessage msg;
    msg.sender = argv[0] ? argv[0] : "";
    msg.text = argv[1] ? argv[1] : "";
    msg.timestamp = argv[2] ? argv[2] : "";
    history->push_back(msg);
    return 0;
}

// Инициализация базы данных и создание всех необходимых таблиц.
void db_init()
{
    if (sqlite3_open(DB_PATH, &db_main) != SQLITE_OK)
    {
        LOG(LogLevel::L_ERROR, "Can't open main database " << DB_PATH << ": " << sqlite3_errmsg(db_main));
        return;
    }
    LOG(LogLevel::INFO, "Main DB " << DB_PATH << " opened successfully.");

    // Создание таблицы applications
    const char *apps_sql = "CREATE TABLE IF NOT EXISTS applications ("
                           "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "USER_ID INTEGER NOT NULL, TARIFF TEXT NOT NULL, PRICE TEXT NOT NULL,"
                           "NAME TEXT NOT NULL, PHONE TEXT NOT NULL, MESSENGER TEXT,"
                           "EMAIL TEXT, ADDRESS TEXT NOT NULL, FLYER_CODE TEXT, STATUS TEXT DEFAULT 'Новая',"
                           "TIMESTAMP DATETIME DEFAULT CURRENT_TIMESTAMP,"
                           "CHAT_STATUS TEXT DEFAULT 'New',"
                           "CHAT_ADMIN_ID INTEGER DEFAULT 0,"
                           "CHAT_POSTPONED_UNTIL DATETIME);";
    if (sqlite3_exec(db_main, apps_sql, 0, 0, 0) != SQLITE_OK)
    {
        LOG(LogLevel::L_ERROR, "Failed to create applications table: " << sqlite3_errmsg(db_main));
    }
    else
    {
        LOG(LogLevel::INFO, "Applications table initialized successfully.");
    }

    // Создание таблицы admins
    const char *admins_sql = "CREATE TABLE IF NOT EXISTS admins ("
                             "USER_ID INTEGER PRIMARY KEY NOT NULL,"
                             "NAME TEXT NOT NULL,"
                             "TRADE_POINT TEXT NOT NULL,"
                             "IS_APPROVED BOOLEAN NOT NULL DEFAULT 0);";
    if (sqlite3_exec(db_main, admins_sql, 0, 0, 0) != SQLITE_OK)
    {
        LOG(LogLevel::L_ERROR, "Failed to create admins table: " << sqlite3_errmsg(db_main));
    }
    else
    {
        LOG(LogLevel::INFO, "Admins table initialized successfully.");
    }

    // Создание таблицы sessions
    const char *session_sql = "CREATE TABLE IF NOT EXISTS sessions ("
                              "USER_ID INTEGER PRIMARY KEY NOT NULL,"
                              "STATE INTEGER NOT NULL);";
    if (sqlite3_exec(db_main, session_sql, 0, 0, 0) != SQLITE_OK)
    {
        LOG(LogLevel::L_ERROR, "Failed to create sessions table: " << sqlite3_errmsg(db_main));
    }
    else
    {
        LOG(LogLevel::INFO, "Sessions table initialized successfully.");
    }

    // Создание таблицы conversations для хранения переписки
    const char *conv_sql = "CREATE TABLE IF NOT EXISTS conversations ("
                           "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "APPLICATION_ID INTEGER NOT NULL,"
                           "SENDER TEXT NOT NULL,"
                           "MESSAGE TEXT NOT NULL,"
                           "TIMESTAMP DATETIME DEFAULT CURRENT_TIMESTAMP);";
    if (sqlite3_exec(db_main, conv_sql, 0, 0, 0) != SQLITE_OK)
    {
        LOG(LogLevel::L_ERROR, "Failed to create conversations table: " << sqlite3_errmsg(db_main));
    }
    else
    {
        LOG(LogLevel::INFO, "Conversations table initialized successfully.");
    }
    db_init_bot_settings();
}

// Закрытие соединения с базой данных.
void db_close()
{
    sqlite3_close(db_main);
    LOG(LogLevel::INFO, "Main DB " << DB_PATH << " closed successfully.");
}

// Инициализация таблицы настроек бота.
void db_init_bot_settings()
{
    const char *sql = "CREATE TABLE IF NOT EXISTS bot_settings ("
                      "KEY TEXT PRIMARY KEY NOT NULL,"
                      "VALUE INTEGER NOT NULL);";
    char *errMsg = 0;
    int rc = sqlite3_exec(db_main, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        LOG(LogLevel::L_ERROR, "SQL error creating bot_settings table: " << errMsg);
        sqlite3_free(errMsg);
    }
    else
    {
        LOG(LogLevel::INFO, "bot_settings table checked/created successfully.");
        sql = "INSERT OR IGNORE INTO bot_settings (KEY, VALUE) VALUES ('is_active', 1);";
        rc = sqlite3_exec(db_main, sql, 0, 0, &errMsg);
        if (rc != SQLITE_OK)
        {
            LOG(LogLevel::L_ERROR, "SQL error inserting default bot_status: " << errMsg);
            sqlite3_free(errMsg);
        }
        else
        {
            LOG(LogLevel::INFO, "Default bot status 'is_active' ensured.");
        }
    }
}

// Получение текущего статуса бота (активен/неактивен).
bool db_get_bot_status()
{
    const char *sql = "SELECT VALUE FROM bot_settings WHERE KEY = 'is_active';";
    sqlite3_stmt *stmt;
    bool status = false;
    if (sqlite3_prepare_v2(db_main, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            status = (sqlite3_column_int(stmt, 0) == 1);
        }
        else
        {
            LOG(LogLevel::L_WARNING, "Bot status 'is_active' not found in bot_settings table. Defaulting to inactive.");
        }
    }
    else
    {
        LOG(LogLevel::L_ERROR, "Failed to prepare SQL statement for db_get_bot_status: " << sqlite3_errmsg(db_main));
    }
    sqlite3_finalize(stmt);
    return status;
}

// Установка статуса бота (активен/неактивен).
void db_set_bot_status(bool active_status)
{
    const char *sql = "INSERT OR REPLACE INTO bot_settings (KEY, VALUE) VALUES ('is_active', ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_main, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, active_status ? 1 : 0);
        int rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            LOG(LogLevel::L_ERROR, "SQL error updating bot status: " << sqlite3_errmsg(db_main));
        }
        else
        {
            LOG(LogLevel::INFO, "Bot status set to: " << (active_status ? "ACTIVE" : "INACTIVE"));
        }
    }
    else
    {
        LOG(LogLevel::L_ERROR, "Failed to prepare SQL statement for db_set_bot_status: " << sqlite3_errmsg(db_main));
    }
    sqlite3_finalize(stmt);
}

// Добавление новой заявки в базу.
void db_add_application(int64_t user_id, const UserData &data, const std::string &full_address, int total_monthly)
{
    std::string price_str = std::to_string(total_monthly) + " ₽/мес";
    char *sql = sqlite3_mprintf("INSERT INTO applications (USER_ID,TARIFF,PRICE,NAME,PHONE,MESSENGER,EMAIL,ADDRESS,FLYER_CODE) VALUES (%lld, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q);",
                                (long long int)user_id, data.final_tariff_string.c_str(), price_str.c_str(), data.name.c_str(), data.phone.c_str(),
                                data.preferred_messenger.c_str(), data.email.c_str(), full_address.c_str(), data.flyer_code.c_str());
    if (sqlite3_exec(db_main, sql, 0, 0, 0) != SQLITE_OK)
    {
        LOG(LogLevel::L_ERROR, "Failed to add application to DB: " << sqlite3_errmsg(db_main));
    }
    sqlite3_free(sql);
}

// Получение списка заявок для конкретного пользователя.
std::string db_get_my_apps(int64_t user_id)
{
    std::string result = "📂 *Ваши оставленные заявки:*\n\n";
    std::string sql = "SELECT TARIFF, PRICE, ADDRESS, STATUS, strftime('%Y-%m-%d %H:%M', TIMESTAMP) FROM applications WHERE USER_ID = " + std::to_string(user_id) + " ORDER BY ID DESC;";
    sqlite3_exec(db_main, sql.c_str(), db_my_apps_callback, &result, 0);
    if (result == "📂 *Ваши оставленные заявки:*\n\n")
    {
        return "Вы еще не оставляли заявок.";
    }
    return result;
}

// Получение списка заявок для конкретной торговой точки.
std::string db_get_apps_by_trade_point(const std::string &trade_point_code)
{
    std::string result = "👑 *Заявки для точки " + trade_point_code + ":*\n\n";
    std::string sql = "SELECT ID, USER_ID, TARIFF, NAME, PRICE, PHONE, EMAIL, ADDRESS, STATUS, strftime('%Y-%m-%d %H:%M', TIMESTAMP) FROM applications WHERE FLYER_CODE = ? ORDER BY ID DESC;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_main, sql.c_str(), -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, trade_point_code.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            char *cols[10];
            for (int i = 0; i < 10; ++i)
            {
                cols[i] = (char *)sqlite3_column_text(stmt, i);
            }
            db_admin_callback(&result, 10, cols, nullptr);
        }
    }
    sqlite3_finalize(stmt);
    if (result == "👑 *Заявки для точки " + trade_point_code + ":*\n\n")
    {
        return "Для точки " + trade_point_code + " заявок пока нет.";
    }
    return result;
}

// Получение данных о заявках для отчета.
std::vector<ApplicationDataForReport> db_get_apps_data_for_report(const std::string &trade_point_code)
{
    std::vector<ApplicationDataForReport> results;
    std::string sql = "SELECT ID, USER_ID, TARIFF, NAME, PRICE, PHONE, EMAIL, ADDRESS, strftime('%Y-%m-%d %H:%M', TIMESTAMP), CHAT_STATUS FROM applications WHERE FLYER_CODE = ? ORDER BY ID DESC;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_main, sql.c_str(), -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, trade_point_code.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            ApplicationDataForReport app_data;
            app_data.id = sqlite3_column_int64(stmt, 0);
            app_data.user_id = sqlite3_column_int64(stmt, 1);
            app_data.tariff = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            app_data.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            app_data.price = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            app_data.phone = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
            app_data.email = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
            app_data.address = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
            app_data.timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
            app_data.chat_status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9));
            results.push_back(app_data);
        }
    }
    sqlite3_finalize(stmt);
    return results;
}

// Получение всех заявок для API
std::vector<ApplicationDataForReport> db_get_all_applications()
{
    std::vector<ApplicationDataForReport> results;
    std::string sql = "SELECT ID, USER_ID, TARIFF, NAME, PRICE, PHONE, EMAIL, ADDRESS, strftime('%Y-%m-%d %H:%M', TIMESTAMP), STATUS FROM applications ORDER BY ID DESC;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_main, sql.c_str(), -1, &stmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            ApplicationDataForReport app_data;
            app_data.id = sqlite3_column_int64(stmt, 0);
            app_data.user_id = sqlite3_column_int64(stmt, 1);
            const char *tariff = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            app_data.tariff = tariff ? tariff : "";
            const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            app_data.name = name ? name : "";
            const char *price = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            app_data.price = price ? price : "";
            const char *phone = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
            app_data.phone = phone ? phone : "";
            const char *email = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
            app_data.email = email ? email : "";
            const char *address = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
            app_data.address = address ? address : "";
            const char *timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
            app_data.timestamp = timestamp ? timestamp : "";
            const char *status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9));
            app_data.chat_status = status ? status : "";
            results.push_back(app_data);
        }
    }
    sqlite3_finalize(stmt);
    return results;
}

// Получение заявки по ID
std::optional<ApplicationDataForReport> db_get_application_by_id(int64_t app_id)
{
    std::string sql = "SELECT ID, USER_ID, TARIFF, NAME, PRICE, PHONE, EMAIL, ADDRESS, strftime('%Y-%m-%d %H:%M', TIMESTAMP), STATUS FROM applications WHERE ID = ?;";
    sqlite3_stmt *stmt;
    std::optional<ApplicationDataForReport> result = std::nullopt;

    if (sqlite3_prepare_v2(db_main, sql.c_str(), -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int64(stmt, 1, app_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            ApplicationDataForReport app_data;
            app_data.id = sqlite3_column_int64(stmt, 0);
            app_data.user_id = sqlite3_column_int64(stmt, 1);
            const char *tariff = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            app_data.tariff = tariff ? tariff : "";
            const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            app_data.name = name ? name : "";
            const char *price = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            app_data.price = price ? price : "";
            const char *phone = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
            app_data.phone = phone ? phone : "";
            const char *email = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
            app_data.email = email ? email : "";
            const char *address = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
            app_data.address = address ? address : "";
            const char *timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
            app_data.timestamp = timestamp ? timestamp : "";
            const char *status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9));
            app_data.chat_status = status ? status : "";

            result = app_data;
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

// Обновление статуса заявки.
void db_update_application_status(long long application_id, ApplicationStatus status)
{
    std::string status_str = statusToString(status);
    char *sql = sqlite3_mprintf("UPDATE applications SET STATUS = %Q WHERE ID = %lld;",
                                status_str.c_str(), application_id);
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
}

// Добавление запроса на админство.
void db_add_admin_request(int64_t user_id, const std::string &name, const std::string &trade_point)
{
    char *sql = sqlite3_mprintf("INSERT OR REPLACE INTO admins (USER_ID, NAME, TRADE_POINT, IS_APPROVED) VALUES (%lld, %Q, %Q, 0);",
                                (long long)user_id, name.c_str(), trade_point.c_str());
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
}

// Одобрение запроса на админство.
void db_approve_admin(int64_t user_id)
{
    char *sql = sqlite3_mprintf("UPDATE admins SET IS_APPROVED = 1 WHERE USER_ID = %lld;", (long long)user_id);
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
}

// Проверка, является ли пользователь одобренным администратором.
bool db_is_admin_approved(int64_t user_id, std::string &trade_point)
{
    std::string sql = "SELECT TRADE_POINT FROM admins WHERE USER_ID = ? AND IS_APPROVED = 1;";
    sqlite3_stmt *stmt;
    bool approved = false;
    if (sqlite3_prepare_v2(db_main, sql.c_str(), -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int64(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            trade_point = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            approved = true;
        }
    }
    sqlite3_finalize(stmt);
    return approved;
}

// Получение всех ожидающих одобрения администраторов.
std::vector<AdminRequestData> db_get_pending_admin_requests()
{
    std::vector<AdminRequestData> requests;
    const char *sql = "SELECT USER_ID, NAME, TRADE_POINT FROM admins WHERE IS_APPROVED = 0;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_main, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            AdminRequestData req;
            req.user_id = sqlite3_column_int64(stmt, 0);
            req.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            req.trade_point = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            requests.push_back(req);
        }
    }
    sqlite3_finalize(stmt);
    return requests;
}

// Отклонение запроса на админство.
void db_decline_admin_request(int64_t user_id)
{
    char *sql = sqlite3_mprintf("DELETE FROM admins WHERE USER_ID = %lld;", (long long)user_id);
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
}

// Получение ID администраторов по коду торговой точки.
std::vector<int64_t> db_get_admin_ids_by_trade_point(const std::string &trade_point)
{
    std::vector<int64_t> admin_ids;
    const char *sql = "SELECT USER_ID FROM admins WHERE TRADE_POINT = ? AND IS_APPROVED = 1;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_main, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, trade_point.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            admin_ids.push_back(sqlite3_column_int64(stmt, 0));
        }
    }
    sqlite3_finalize(stmt);
    return admin_ids;
}

// Получение всех одобренных администраторов.
std::vector<AdminRequestData> db_get_all_admins()
{
    std::vector<AdminRequestData> admins;
    const char *sql = "SELECT USER_ID, NAME, TRADE_POINT FROM admins WHERE IS_APPROVED = 1;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_main, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            AdminRequestData admin;
            admin.user_id = sqlite3_column_int64(stmt, 0);
            admin.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            admin.trade_point = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            admins.push_back(admin);
        }
    }
    sqlite3_finalize(stmt);
    return admins;
}

// Добавление администратора вручную.
void db_add_admin_manual(int64_t user_id, const std::string &name, const std::string &trade_point)
{
    char *sql = sqlite3_mprintf("INSERT OR REPLACE INTO admins (USER_ID, NAME, TRADE_POINT, IS_APPROVED) VALUES (%lld, %Q, %Q, 1);",
                                (long long)user_id, name.c_str(), trade_point.c_str());
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
}

// Удаление администратора.
void db_delete_admin(int64_t user_id)
{
    char *sql = sqlite3_mprintf("DELETE FROM admins WHERE USER_ID = %lld;", (long long)user_id);
    sqlite3_exec(db_main, sql, nullptr, nullptr, nullptr);
    sqlite3_free(sql);
    db_delete_session(user_id);
}

// Проверка, существует ли администратор.
bool db_admin_exists(int64_t user_id)
{
    std::string sql = "SELECT COUNT(*) FROM admins WHERE USER_ID = ? AND IS_APPROVED = 1;";
    sqlite3_stmt *stmt;
    bool exists = false;
    if (sqlite3_prepare_v2(db_main, sql.c_str(), -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int64(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_int(stmt, 0) > 0)
            {
                exists = true;
            }
        }
    }
    else
    {
        LOG(LogLevel::L_ERROR, "Failed to prepare SQL statement for db_admin_exists: " << sqlite3_errmsg(db_main));
    }
    sqlite3_finalize(stmt);
    return exists;
}

// Получение режима работы администратора.
AdminWorkMode db_get_admin_work_mode(int64_t user_id)
{
    const char *sql = "SELECT STATE FROM sessions WHERE USER_ID = ?;";
    sqlite3_stmt *stmt;
    AdminWorkMode mode = AdminWorkMode::UNKNOWN;
    if (sqlite3_prepare_v2(db_main, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int64(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int stored_state = sqlite3_column_int(stmt, 0);
            if (stored_state >= 100)
            {
                mode = static_cast<AdminWorkMode>(stored_state - 100);
            }
            else
            {
                LOG(LogLevel::L_WARNING, "User " << user_id << " has a non-admin session state (" << stored_state << ") when admin work mode was requested.");
            }
        }
        else
        {
            LOG(LogLevel::INFO, "No session state found for user " << user_id << ". Defaulting to UNKNOWN.");
        }
    }
    else
    {
        LOG(LogLevel::L_ERROR, "Failed to prepare SQL statement for db_get_admin_work_mode: " << sqlite3_errmsg(db_main));
    }
    sqlite3_finalize(stmt);
    return mode;
}

// Сохранение состояния пользователя.
void db_save_user_state(int64_t user_id, UserState state)
{
    char *sql = sqlite3_mprintf("INSERT OR REPLACE INTO sessions (USER_ID, STATE) VALUES (%lld, %d);",
                                (long long)user_id, static_cast<int>(state));
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
}

// Загрузка состояний пользователей.
void db_load_user_states(std::map<int64_t, UserData> &session_map)
{
    const char *sql = "SELECT USER_ID, STATE FROM sessions;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_main, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int64_t user_id = sqlite3_column_int64(stmt, 0);
            auto state = static_cast<UserState>(sqlite3_column_int(stmt, 1));
            session_map[user_id].state = state;
        }
    }
    sqlite3_finalize(stmt);
    printf("Loaded %zu user states from DB.\n", session_map.size());
}

// Сохранение режима работы администратора.
void db_save_admin_work_mode(int64_t user_id, AdminWorkMode mode)
{
    char *sql = sqlite3_mprintf("INSERT OR REPLACE INTO sessions (USER_ID, STATE) VALUES (%lld, %d);",
                                (long long)user_id, static_cast<int>(mode) + 100);
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
}

// Загрузка режимов работы администраторов.
void db_load_admin_work_modes(std::map<int64_t, AdminWorkMode> &mode_map)
{
    const char *sql = "SELECT USER_ID, STATE FROM sessions WHERE STATE >= 100;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_main, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int64_t user_id = sqlite3_column_int64(stmt, 0);
            auto mode = static_cast<AdminWorkMode>(sqlite3_column_int(stmt, 1) - 100);
            mode_map[user_id] = mode;
        }
    }
    sqlite3_finalize(stmt);
    printf("Loaded %zu super admin modes from DB.\n", mode_map.size());
}

// Удаление сессии пользователя.
void db_delete_session(int64_t user_id)
{
    char *sql = sqlite3_mprintf("DELETE FROM sessions WHERE USER_ID = %lld;", (long long)user_id);
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
}

// Конвертация статуса заявки в строку.
std::string statusToString(ApplicationStatus status)
{
    switch (status)
    {
    case ApplicationStatus::New:
        return "Новая";
    case ApplicationStatus::InProgress:
        return "В работе";
    case ApplicationStatus::Done:
        return "Выполнена";
    case ApplicationStatus::Cancelled:
        return "Отменена";
    default:
        return "Неизвестен";
    }
}

// Конвертация статуса чата в строку.
std::string chatStatusToString(ChatStatus status)
{
    switch (status)
    {
    case ChatStatus::New:
        return "New";
    case ChatStatus::InProgress:
        return "In Progress";
    case ChatStatus::Postponed:
        return "Postponed";
    case ChatStatus::Completed:
        return "Completed";
    default:
        return "Unknown";
    }
}

// Обновление статуса чата.
void db_update_chat_status(long long application_id, ChatStatus status, int64_t admin_id, const std::string &postponed_until)
{
    std::string status_str = chatStatusToString(status);
    char *sql = nullptr;
    if (status == ChatStatus::Postponed && !postponed_until.empty())
    {
        sql = sqlite3_mprintf("UPDATE applications SET CHAT_STATUS = %Q, CHAT_ADMIN_ID = %lld, CHAT_POSTPONED_UNTIL = %Q WHERE ID = %lld;",
                              status_str.c_str(), (long long)admin_id, postponed_until.c_str(), application_id);
    }
    else
    {
        sql = sqlite3_mprintf("UPDATE applications SET CHAT_STATUS = %Q, CHAT_ADMIN_ID = %lld, CHAT_POSTPONED_UNTIL = NULL WHERE ID = %lld;",
                              status_str.c_str(), (long long)admin_id, application_id);
    }
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
    LOG(LogLevel::INFO, "Updated chat status for app ID " << application_id << " to " << status_str);
}

// Добавление сообщения в историю чата.
void db_add_chat_message(long long application_id, const ChatMessage &message)
{
    char *sql = sqlite3_mprintf("INSERT INTO conversations (APPLICATION_ID, SENDER, MESSAGE) VALUES (%lld, %Q, %Q);",
                                application_id, message.sender.c_str(), message.text.c_str());
    sqlite3_exec(db_main, sql, 0, 0, 0);
    sqlite3_free(sql);
}

// Получение истории чата.
std::vector<ChatMessage> db_get_chat_history(long long application_id)
{
    std::vector<ChatMessage> history;
    char *sql = sqlite3_mprintf("SELECT SENDER, MESSAGE, TIMESTAMP FROM conversations WHERE APPLICATION_ID = %lld ORDER BY TIMESTAMP ASC;", application_id);
    sqlite3_exec(db_main, sql, db_chat_history_callback, &history, 0);
    sqlite3_free(sql);
    return history;
}

// Получение ID администратора, который ведет чат.
int64_t db_get_chat_admin(long long application_id)
{
    const char *sql = "SELECT CHAT_ADMIN_ID FROM applications WHERE ID = ?;";
    sqlite3_stmt *stmt;
    int64_t admin_id = 0;
    if (sqlite3_prepare_v2(db_main, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int64(stmt, 1, application_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            admin_id = sqlite3_column_int64(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return admin_id;
}