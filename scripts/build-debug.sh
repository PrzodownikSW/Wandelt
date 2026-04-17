#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."

echo "Building Debug..."
cmake --build build/Debug
