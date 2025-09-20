#pragma once
#include <nlohmann/json.hpp>

namespace Buttons {
    // Клавиатура для стартового сообщения
    nlohmann::json get_start_keyboard();
    // Клавиатура главного меню
    nlohmann::json get_main_menu_keyboard();
}