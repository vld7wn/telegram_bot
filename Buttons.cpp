#include "Buttons.h"

nlohmann::json Buttons::get_start_keyboard() {
    return {
                {"inline_keyboard", {{
                    {{"text", "Инструкция изучена"}, {"callback_data", "instruction_seen"}}
                }}}
    };
}

nlohmann::json Buttons::get_main_menu_keyboard() {
    // IMPORTANT: Replace this URL with your HTTPS address from ngrok or localtunnel
    std::string web_app_url = "https://tg-my-site.ru";

    return {
                {"inline_keyboard", {{
                    {{"text", "📝 Оставить заявку"}, {"web_app", {{"url", web_app_url}}}},
                    {{"text", "🔍 Проверить свою заявку"}, {"callback_data", "check_request"}}
                }}}
    };
}