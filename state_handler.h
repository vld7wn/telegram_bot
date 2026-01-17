#pragma once
#include <tgbot/tgbot.h>
#include <memory>
#include <unordered_map>
#include <functional>

// Forward declarations
struct UserData;
enum class UserState;

/**
 * IStateHandler - базовый интерфейс для обработчиков состояний
 * Каждое состояние пользователя имеет свой обработчик
 */
class IStateHandler
{
public:
    virtual ~IStateHandler() = default;

    /**
     * Обработка текстового сообщения от пользователя
     * @return true если сообщение обработано, false если нужно передать дальше
     */
    virtual bool handleMessage(TgBot::Bot &bot, TgBot::Message::Ptr message, UserData &user) = 0;

    /**
     * Обработка callback query (нажатие на inline-кнопку)
     * @return true если callback обработан, false если нужно передать дальше
     */
    virtual bool handleCallback(TgBot::Bot &bot, TgBot::CallbackQuery::Ptr query, UserData &user) = 0;

    /**
     * Обработка кнопки "Назад"
     * @return true если обработано, false для возврата в главное меню
     */
    virtual bool handleBack(TgBot::Bot &bot, int64_t chat_id, UserData &user) = 0;
};

/**
 * StateHandlerRegistry - реестр обработчиков состояний (Singleton)
 */
class StateHandlerRegistry
{
public:
    using HandlerPtr = std::shared_ptr<IStateHandler>;
    using HandlerFactory = std::function<HandlerPtr()>;

    static StateHandlerRegistry &instance();

    void registerHandler(UserState state, HandlerFactory factory);
    HandlerPtr getHandler(UserState state);
    bool hasHandler(UserState state) const;

private:
    StateHandlerRegistry() = default;
    std::unordered_map<int, HandlerFactory> factories_;
    std::unordered_map<int, HandlerPtr> handlers_; // кеш
};

// Макрос для регистрации обработчиков
#define REGISTER_STATE_HANDLER(state, handler_class)                                                                         \
    namespace                                                                                                                \
    {                                                                                                                        \
        struct handler_class##_Registrar                                                                                     \
        {                                                                                                                    \
            handler_class##_Registrar()                                                                                      \
            {                                                                                                                \
                StateHandlerRegistry::instance().registerHandler(state, []() { return std::make_shared<handler_class>(); }); \
            }                                                                                                                \
        } handler_class##_registrar_instance;                                                                                \
    }
