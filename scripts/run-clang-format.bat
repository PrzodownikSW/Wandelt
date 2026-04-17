@echo off
pushd "%~dp0.."

echo Running clang-format...
for /r src %%f in (*.cpp *.hpp) do (
    echo Formatting %%f
    clang-format -i "%%f"
)
for /r tests %%f in (*.cpp *.hpp) do (
    echo Formatting %%f
    clang-format -i "%%f"
)

echo Done.
popd
pause
