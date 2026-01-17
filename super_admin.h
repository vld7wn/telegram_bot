#pragma once
#include <tgbot/tgbot.h>
#include <map>

enum class AdminWorkMode {
    UNKNOWN,
    ADMIN_VIEW,
    APPROVING_ADMINS,
    SA_MANAGE_ADMINS,
    SA_AWAITING_ADD_ID,
    SA_AWAITING_ADD_TP,
    SA_AWAITING_DELETE_ID,
    SA_AWAITING_TP_FOR_VIEW
};

void sendSuperAdminPanel(TgBot::Bot& bot, int64_t chat_id);
void handle_super_admin_message(TgBot::Bot& bot, TgBot::Message::Ptr message);
void sendAdminApprovalList(TgBot::Bot& bot, int64_t chat_id);
void sendAdminManagementPanel(TgBot::Bot& bot, int64_t chat_id);
void sendTradePointSelectionForNewAdmin(TgBot::Bot& bot, int64_t chat_id);
void handle_super_admin_callbacks(TgBot::Bot& bot, TgBot::CallbackQuery::Ptr query);

void sendAllApplicationsOverview(TgBot::Bot& bot, int64_t chat_id);