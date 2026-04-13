@echo off
pushd "%~dp0.."

echo Configuring for Visual Studio 2022...
cmake -B build -G "Visual Studio 17 2022" -A x64

popd
pause
