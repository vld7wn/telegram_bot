# Минимальная требуемая версия CMake
cmake_minimum_required(VERSION 3.10)

# Название проекта
project(telegram_bot CXX)

# Устанавливаем стандарт C++ 26
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ищем установленные библиотеки
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(cpr REQUIRED)
find_package(Boost REQUIRED COMPONENTS system) # Required for Crow

# Создаем исполняемый файл из main.cpp с именем tg_bot_fs
add_executable(tg_bot_fs main.cpp
        Bot.cpp
        TelegramApi.cpp
        Buttons.cpp
        utils/logger.cpp
        #utils/excel_generate.cpp
        #utils/faq_manager.cpp
)

# Подключаем (линкуем) библиотеки к нашему файлу
target_link_libraries(tg_bot_fs PRIVATE
        nlohmann_json::nlohmann_json
        cpr::cpr
        Boost::system
)