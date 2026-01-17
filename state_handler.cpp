#include "state_handler.h"
#include "user_data_types.h"

StateHandlerRegistry &StateHandlerRegistry::instance()
{
    static StateHandlerRegistry registry;
    return registry;
}

void StateHandlerRegistry::registerHandler(UserState state, HandlerFactory factory)
{
    factories_[static_cast<int>(state)] = factory;
}

StateHandlerRegistry::HandlerPtr StateHandlerRegistry::getHandler(UserState state)
{
    int key = static_cast<int>(state);

    // Проверяем кеш
    auto cached = handlers_.find(key);
    if (cached != handlers_.end())
    {
        return cached->second;
    }

    // Создаём новый обработчик
    auto factory = factories_.find(key);
    if (factory != factories_.end())
    {
        auto handler = factory->second();
        handlers_[key] = handler;
        return handler;
    }

    return nullptr;
}

bool StateHandlerRegistry::hasHandler(UserState state) const
{
    return factories_.count(static_cast<int>(state)) > 0;
}
