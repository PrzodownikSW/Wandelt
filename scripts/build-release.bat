@echo off
pushd "%~dp0.."

echo Building Release...
cmake --build build --config Release

popd
pause
