#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."

echo "Configuring with Clang (Debug, Release, Dist)..."

for cfg in Debug Release Dist; do
    cmake -B "build/$cfg" -G "Unix Makefiles" \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_BUILD_TYPE="$cfg" \
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$PWD/bin/$cfg" \
        -DWDT_COMPILE_COMMANDS=ON
done

ln -sf "build/Debug/compile_commands.json" compile_commands.json
