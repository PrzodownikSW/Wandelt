@echo off
setlocal

set ROOT=%~dp0..
set BUILD_DIR=%ROOT%\build\build-cov
set BIN=%ROOT%\bin\Wandelt.exe
set PROFRAW=%BUILD_DIR%\coverage.profraw
set PROFDATA=%BUILD_DIR%\coverage.profdata
set REPORT_DIR=%BUILD_DIR%\coverage-report

echo [1/5] Configuring CMake with coverage flags...
cmake -B "%BUILD_DIR%" -S "%ROOT%" -G "Ninja" ^
    -DCMAKE_C_COMPILER=clang ^
    -DCMAKE_CXX_COMPILER=clang++ ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_C_FLAGS="-fprofile-instr-generate -fcoverage-mapping" ^
    -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping" ^
    -DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate"
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed.
    exit /b 1
)

echo.
echo [2/5] Building Wandelt...
cmake --build "%BUILD_DIR%" --target Wandelt
if %errorlevel% neq 0 (
    echo ERROR: Build failed.
    exit /b 1
)

echo.
echo [3/5] Running tests...
set LLVM_PROFILE_FILE=%PROFRAW%
"%BIN%" -test -no-colors
if %errorlevel% neq 0 (
    echo ERROR: Tests failed.
    exit /b 1
)

echo.
echo [4/5] Merging profile data...
llvm-profdata merge "%PROFRAW%" -o "%PROFDATA%"
if %errorlevel% neq 0 (
    echo ERROR: llvm-profdata merge failed.
    exit /b 1
)

echo.
echo [5/5] Generating coverage report...
echo.
echo --- Summary ---
llvm-cov report "%BIN%" -instr-profile="%PROFDATA%" -sources "%ROOT%\src\Wandelt" -show-region-summary -show-branch-summary
echo.

llvm-cov show "%BIN%" -instr-profile="%PROFDATA%" -sources "%ROOT%\src\Wandelt" ^
    -format=html ^
    -show-line-counts-or-regions ^
    -show-branches=count ^
    -output-dir="%REPORT_DIR%"
if %errorlevel% neq 0 (
    echo ERROR: llvm-cov failed.
    exit /b 1
)

echo.
echo HTML report generated at: %REPORT_DIR%\index.html
start "" "%REPORT_DIR%\index.html"

endlocal
pause
