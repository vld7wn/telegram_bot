#!/bin/bash
# Скрипт установки Telegram-бота на Orange Pi / Armbian
# Запуск: bash install_bot.sh

set -e

echo "=========================================="
echo "  Установка Telegram-бота на Orange Pi"
echo "=========================================="

# 1. Обновление системы и установка зависимостей
echo "[1/5] Установка зависимостей..."
apt update
apt install -y \
    build-essential cmake git \
    libssl-dev libcurl4-openssl-dev \
    libsqlite3-dev libboost-system-dev \
    curl zip unzip tar pkg-config ninja-build

# 2. Установка vcpkg
echo "[2/5] Установка vcpkg (это займёт время)..."
cd /opt
if [ ! -d "vcpkg" ]; then
    git clone https://github.com/Microsoft/vcpkg.git
fi
cd vcpkg
export VCPKG_FORCE_SYSTEM_BINARIES=1
./bootstrap-vcpkg.sh

# 3. Установка библиотек через vcpkg
echo "[3/5] Установка библиотек (tgbot-cpp, nlohmann-json, cpp-httplib)..."
echo "      Это может занять 1-3 часа на ARM..."
./vcpkg install tgbot-cpp nlohmann-json cpp-httplib

# 4. Клонирование и сборка бота
echo "[4/5] Клонирование и сборка бота..."
cd /opt
if [ ! -d "telegram_bot" ]; then
    git clone https://github.com/vld7wn/telegram_bot.git
else
    cd telegram_bot && git pull && cd ..
fi
cd telegram_bot
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release -j$(nproc)

# 5. Создание systemd сервиса
echo "[5/5] Создание systemd сервиса..."
cat > /etc/systemd/system/telegram-bot.service << 'EOF'
[Unit]
Description=MTS Telegram Bot
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/telegram_bot/build
ExecStart=/opt/telegram_bot/build/my_telegram_bot
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable telegram-bot

echo ""
echo "=========================================="
echo "  Установка завершена!"
echo "=========================================="
echo ""
echo "Команды управления:"
echo "  Запустить:    systemctl start telegram-bot"
echo "  Остановить:   systemctl stop telegram-bot"
echo "  Статус:       systemctl status telegram-bot"
echo "  Логи:         journalctl -u telegram-bot -f"
echo ""
echo "Конфиг: /opt/telegram_bot/build/json-cfg/config.json"
echo ""
