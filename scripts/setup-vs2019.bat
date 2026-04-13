@echo off
pushd "%~dp0.."

echo Configuring for Visual Studio 2019...
cmake -B build -G "Visual Studio 16 2019" -A x64

popd
pause
