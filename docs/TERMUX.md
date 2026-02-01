# Запуск Telegram бота на Android (Termux)

Вы можете запустить этого бота прямо на планшете или телефоне через приложение Termux. Это полноценная Linux-среда.

## 1. Установка Termux

1. Скачайте **Termux** из [F-Droid](https://f-droid.org/packages/com.termux/) (версия в Google Play устарела).
2. Запустите приложение.

## 2. Установка Бота

Мы подготовили автоматический скрипт установки.

1. Обновите пакеты:

    ```bash
    pkg update && pkg upgrade
    ```

2. Установите git (если нет):

    ```bash
    pkg install git
    ```

3. Склонируйте репозиторий:

    ```bash
    git clone https://github.com/vld7wn/telegram_bot.git
    cd telegram_bot
    ```

4. Запустите скрипт установки:

    ```bash
    chmod +x scripts/setup_termux.sh
    ./scripts/setup_termux.sh
    ```

Скрипт автоматически:

- Установит компилятор `clang`, `cmake` и `make`.
- Установит библиотеки `boost`, `openssl`, `curl`, `jsoncpp`, `sqlite`.
- Скачает и соберет библиотеку `tgbot-cpp` (это займет время).
- Соберет самого бота.

## 3. Запуск

После успешной сборки:

```bash
cd build
./my_telegram_bot
```

> **Важно:** Убедитесь, что `config.json` настроен. Скрипт установки предложит отредактировать его, если он отсутствует.

## Решение проблем

### Ошибка сборки TgBot

Если `tgbot-cpp` не собирается, убедитесь, что все зависимости установлены:

```bash
pkg install clang cmake make boost libcurl-dev openssl-tool libsqlite nlohmann-json-dev
```

### Доступ к Админке

Бот запустит веб-сервер на порту `8080`.
Откройте в браузере `http://localhost:8080`.
Если хотите открыть с другого устройства в той же Wi-Fi сети, узнайте IP планшета командой `ifconfig` и заходите по `http://IP_ПЛАНШЕТА:8080`.
