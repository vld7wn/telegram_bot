#include "state_handler.h"
#include "handlers/input_handlers.h"
#include "user_data_types.h"

// Регистрация всех обработчиков состояний
namespace
{
    struct HandlerRegistrar
    {
        HandlerRegistrar()
        {
            auto &registry = StateHandlerRegistry::instance();

            // Обработчики ввода данных
            registry.registerHandler(UserState::ENTERING_NAME, []()
                                     { return std::make_shared<EnterNameHandler>(); });
            registry.registerHandler(UserState::ENTERING_PHONE, []()
                                     { return std::make_shared<EnterPhoneHandler>(); });
            registry.registerHandler(UserState::ENTERING_EMAIL, []()
                                     { return std::make_shared<EnterEmailHandler>(); });
            registry.registerHandler(UserState::ENTERING_CITY, []()
                                     { return std::make_shared<EnterCityHandler>(); });
            registry.registerHandler(UserState::ENTERING_STREET, []()
                                     { return std::make_shared<EnterStreetHandler>(); });
            registry.registerHandler(UserState::ENTERING_HOUSE, []()
                                     { return std::make_shared<EnterHouseHandler>(); });
            registry.registerHandler(UserState::ENTERING_HOUSE_BODY, []()
                                     { return std::make_shared<EnterHouseBodyHandler>(); });
            registry.registerHandler(UserState::ENTERING_APARTMENT, []()
                                     { return std::make_shared<EnterApartmentHandler>(); });
        }
    } handler_registrar;
}
