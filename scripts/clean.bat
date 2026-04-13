@echo off
pushd "%~dp0.."

echo Cleaning build and bin directories...
if exist build rmdir /s /q build
if exist bin rmdir /s /q bin

echo Done.
popd
pause
