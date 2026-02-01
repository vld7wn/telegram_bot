#!/bin/bash
set -e # Stop script on any error

# Termux Setup Script for Telegram Bot

echo "🚀 Starting Termux Setup..."

# 1. Update and Install Dependencies
echo "📦 Installing system dependencies..."
pkg update -y
# Try to install boost-headers if available, otherwise just boost (newer Termux might rely on boost-headers package)
pkg install -y clang cmake make git \
    boost boost-headers libcurl openssl libsqlite \
    nlohmann-json binutils || echo "⚠️  boost-headers not found, assuming included in boost..."

# 2. Check/Install TgBot-cpp
if [ ! -d "tgbot-cpp" ]; then
    echo "⬇️ Cloning tgbot-cpp..."
    git clone https://github.com/reo7sp/tgbot-cpp.git
else
    echo "🔄 Updating tgbot-cpp..."
    cd tgbot-cpp
    git pull
    cd ..
fi

echo "⚙️ Building tgbot-cpp (this might take a while)..."
cd tgbot-cpp
cmake . -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -DBOOST_INCLUDEDIR=$PREFIX/include \
    -DBOOST_LIBRARYDIR=$PREFIX/lib
make -j$(nproc)
make install
cd ..

# 3. Build the Bot
echo "🤖 Building My Telegram Bot..."
echo "🧹 Cleaning up previous build..."
rm -rf build
mkdir -p build
cd build

# Configure CMake to use system libraries (Termux)
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DBOOST_INCLUDEDIR=$PREFIX/include \
    -DBOOST_LIBRARYDIR=$PREFIX/lib

# Build
make -j$(nproc)

echo "✅ Build Successful!"
echo "To run the bot:"
echo "  cd build"
echo "  ./my_telegram_bot"
