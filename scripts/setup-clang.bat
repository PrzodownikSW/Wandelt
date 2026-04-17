@echo off
pushd "%~dp0.."

echo Configuring with Clang...
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DWDT_COMPILE_COMMANDS=ON

popd
pause
