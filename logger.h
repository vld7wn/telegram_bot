// logger.h
#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

// Используем префикс, чтобы избежать конфликтов с макросами Windows
enum class LogLevel
{
    INFO,
    L_WARNING, // Изменено с WARNING
    L_ERROR    // Изменено с ERROR
};

class Logger
{
public:
    static Logger &get();
    void setup(const std::string &filename);
    void setMinLevel(LogLevel level);
    void setMinLevel(const std::string &levelStr);
    void log(LogLevel level, const std::string &message);

private:
    Logger();  // Приватный конструктор для Singleton
    ~Logger(); // Приватный деструктор

    std::ofstream logFile;
    std::mutex mtx;
    bool initialized = false;
    LogLevel minLevel = LogLevel::INFO;

    // Закрываем конструктор копирования и оператор присваивания
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    std::string getLogLevelString(LogLevel level);
};

// Макрос LOG теперь должен использовать новые имена
#define LOG(level, message)                 \
    do                                      \
    {                                       \
        std::stringstream ss;               \
        ss << message;                      \
        Logger::get().log(level, ss.str()); \
    } while (0)
