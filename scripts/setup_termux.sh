#!/bin/bash

# Termux Setup Script for Telegram Bot

echo "🚀 Starting Termux Setup..."

# 1. Update and Install Dependencies
echo "📦 Installing system dependencies..."
pkg update -y
pkg install -y clang cmake make git \
    boost libcurl openssl libsqlite \
    nlohmann-json binutils || { echo "❌ Failed to install dependencies"; exit 1; }

# 2. Check/Install TgBot-cpp
if [ ! -d "tgbot-cpp" ]; then
    echo "⬇️ Cloning tgbot-cpp..."
    git clone https://github.com/reo7sp/tgbot-cpp.git
fi

echo "⚙️ Building tgbot-cpp (this might take a while)..."
cd tgbot-cpp
cmake . -DCMAKE_INSTALL_PREFIX=$PREFIX
make -j$(nproc)
make install
cd ..

# 3. Build the Bot
echo "🤖 Building My Telegram Bot..."
mkdir -p build
cd build

# Configure CMake to use system libraries (Termux)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "✅ Build Successful!"
    echo "To run the bot:"
    echo "  cd build"
    echo "  ./my_telegram_bot"
else
    echo "❌ Build Failed. Please check errors above."
fi
