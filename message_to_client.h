#pragma once

#include <tgbot/tgbot.h>
#include <string>
#include "user_data_types.h"

void handle_client_reply(TgBot::Bot& bot, TgBot::Message::Ptr message);
void sendAdminPanel(TgBot::Bot& bot, int64_t chat_id);