# My Telegram Bot

Telegram-бот для обработки заявок на подключение интернета/ТВ с системой администрирования.

## Требования

- **C++17** компилятор (MinGW-w64, GCC, MSVC)
- **CMake** >= 3.15
- **vcpkg** для управления зависимостями

### Зависимости (устанавливаются через vcpkg)

```bash
vcpkg install tgbot-cpp nlohmann-json sqlite3 xlnt boost-system openssl curl
```

## Сборка

### 1. Установите переменную окружения VCPKG_ROOT

```powershell
# Windows PowerShell
$env:VCPKG_ROOT = "D:\vcpkg"  # Путь к вашей установке vcpkg
```

### 2. Создайте папку сборки и соберите проект

```bash
mkdir cmake-build-debug
cd cmake-build-debug
cmake .. -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-static
cmake --build . --target my_telegram_bot
```

### 3. Запустите бота

```bash
./my_telegram_bot.exe
```

## Конфигурация

Создайте файл `json-cfg/config.json`:

```json
{
  "bot_token": "YOUR_BOT_TOKEN_HERE",
  "main_admin_id": 123456789,
  "log_level": "INFO",
  "log_file": "logs/bot.log"
}
```

### Параметры конфигурации

| Параметр | Описание | Обязательный |
|----------|----------|--------------|
| `bot_token` | Токен Telegram бота от @BotFather | Да |
| `main_admin_id` | Telegram ID главного администратора | Да |
| `log_level` | Уровень логирования: `INFO`, `WARNING`, `ERROR` | Нет (по умолчанию: `INFO`) |
| `log_file` | Путь к файлу логов | Нет (по умолчанию: `logs/bot.log`) |

## Структура проекта

```
my_telegram_bot/
├── main.cpp              # Точка входа
├── database.cpp/h        # Работа с SQLite
├── application_flow.cpp/h # Логика заявок клиентов
├── admin_panel.cpp/h     # Панель администратора
├── super_admin.cpp/h     # Панель главного админа
├── tariff_manager.cpp/h  # Управление тарифами
├── config.cpp/h          # Конфигурация
├── logger.cpp/h          # Логирование
├── json-cfg/             # JSON конфигурации
│   ├── config.json       # Основная конфигурация
│   ├── tariff_plann.json # Тарифные планы
│   ├── trade_points.json # Торговые точки
│   └── faq.json          # FAQ
├── db/                   # База данных SQLite
└── logs/                 # Логи
```

## Использование

1. Отправьте `/start` боту в Telegram
2. Клиенты могут подать заявку на подключение
3. Администраторы получают уведомления о новых заявках
4. Главный админ управляет всеми администраторами

## Graceful Shutdown

Для корректной остановки бота нажмите `Ctrl+C`. Бот сохранит все данные перед выходом.
