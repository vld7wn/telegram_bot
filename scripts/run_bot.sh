#!/bin/bash
# Script to run the bot from the project root

# Determine the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
BUILD_DIR="$PROJECT_ROOT/build"

echo "🚀 Starting Bot..."
if [ -f "$BUILD_DIR/my_telegram_bot" ]; then
    cd "$BUILD_DIR"
    ./my_telegram_bot
else
    echo "❌ Bot binary not found in $BUILD_DIR"
    echo "   Please run ./scripts/setup_termux.sh first to build the bot."
    exit 1
fi
