#include "message_to_client.h"
#include "main.h"
#include "admin_panel.h" // Для sendAdminPanel()
#include "logger.h"
#include "user_data_types.h" // Для UserData, UserState
#include <tgbot/tgbot.h>
#include <sstream>

void handle_client_reply(TgBot::Bot& bot, TgBot::Message::Ptr message) {
    int64_t chat_id = message->chat->id;
    UserData& user = user_session_data[chat_id];
    
    LOG(LogLevel::INFO, "Admin (ID: " << chat_id << ") is in ADMIN_REPLYING_TO_USER state. Received message: '" << message->text << "'");

    if (message->text == "Отмена") {
        bot.getApi().sendMessage(chat_id, "Отправка сообщения отменена.");
        sendAdminPanel(bot, chat_id);
        LOG(LogLevel::INFO, "Admin (ID: " << chat_id << ") cancelled message sending.");
        return;
    }

    try {
        bot.getApi().sendMessage(user.reply_to_user_id, "Сообщение от администратора:\n\n" + message->text);
        bot.getApi().sendMessage(chat_id, "✅ Сообщение успешно отправлено клиенту.");
        LOG(LogLevel::INFO, "Message successfully sent from admin (ID: " << chat_id << ") to client (ID: " << user.reply_to_user_id << ").");
    } catch (const TgBot::TgException& e) {
        bot.getApi().sendMessage(chat_id, "❌ Не удалось отправить сообщение. Возможно, пользователь заблокировал бота.");
        LOG(LogLevel::L_ERROR, "Failed to send message from admin (ID: " << chat_id << ") to client (ID: " << user.reply_to_user_id << "): " << e.what());
    }

    sendAdminPanel(bot, chat_id);
    return;
}