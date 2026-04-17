#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."

echo "Cleaning build and bin directories..."
rm -rf build bin compile_commands.json

echo "Done."
