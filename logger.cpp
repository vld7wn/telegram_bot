#include "logger.h"
#include <iostream> // Добавлено для std::cerr
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cctype>

Logger::Logger() : initialized(false), minLevel(LogLevel::INFO) {
    // Конструктор по умолчанию. Инициализация файла происходит в setup().
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

Logger& Logger::get() {
    static Logger instance;
    return instance;
}

void Logger::setup(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mtx);
    if (logFile.is_open()) {
        logFile.close();
    }
    logFile.open(filename, std::ios::out | std::ios::app);
    if (logFile.is_open()) {
        initialized = true;
    } else {
        std::cerr << "LOGGER ERROR: Could not open log file: " << filename << std::endl;
    }
}

void Logger::setMinLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mtx);
    minLevel = level;
}

void Logger::setMinLevel(const std::string& levelStr) {
    std::string upper = levelStr;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    if (upper == "INFO") {
        setMinLevel(LogLevel::INFO);
    } else if (upper == "WARNING") {
        setMinLevel(LogLevel::L_WARNING);
    } else if (upper == "ERROR") {
        setMinLevel(LogLevel::L_ERROR);
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);

    // Фильтрация по минимальному уровню
    if (static_cast<int>(level) < static_cast<int>(minLevel)) {
        return;
    }

    if (!logFile.is_open() || !initialized) {
        std::cerr << "LOGGER ERROR: Log file not open or not initialized. Message: ["
                  << getLogLevelString(level) << "] " << message << std::endl;
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    logFile << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << " ";

    logFile << getLogLevelString(level) << ": " << message << std::endl;
    logFile.flush(); // Принудительно сбрасываем буфер
}

std::string Logger::getLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "[INFO]";
        case LogLevel::L_WARNING: return "[WARNING]";
        case LogLevel::L_ERROR: return "[ERROR]";
    }
    return "[UNKNOWN]";
}