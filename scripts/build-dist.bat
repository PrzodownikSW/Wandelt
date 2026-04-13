@echo off
pushd "%~dp0.."

echo Building Dist...
cmake --build build --config Dist

popd
pause
