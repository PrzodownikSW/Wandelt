#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."

echo "Building Dist..."
cmake --build build/Dist
