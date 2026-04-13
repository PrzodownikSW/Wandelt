@echo off
pushd "%~dp0.."

echo Building Debug...
cmake --build build --config Debug

popd
pause
