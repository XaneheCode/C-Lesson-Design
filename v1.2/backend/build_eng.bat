@echo off
echo ========================================
echo   Stationery Shop Management System
echo   Build Script
echo ========================================

cd /d "%~dp0"

echo.
echo [1/3] Compiling data generator...
gcc -Wall -O2 -o generate_data.exe generate_data.c
if errorlevel 1 (
    echo FAILED: Could not compile data generator!
    echo Make sure gcc is installed and in PATH.
    pause
    exit /b 1
)
echo       OK

echo.
echo [2/3] Generating mock data...
cd ..
backend\generate_data.exe
cd backend
if errorlevel 1 (
    echo FAILED: Could not generate data!
    pause
    exit /b 1
)
echo       OK

echo.
echo [3/3] Compiling server...
gcc -Wall -O2 -o server.exe server.c data.c utils.c product.c sale.c purchase.c stats.c assistant.c -lws2_32 -lwinhttp
if errorlevel 1 (
    echo FAILED: Could not compile server!
    pause
    exit /b 1
)
echo       OK

echo.
echo ========================================
echo   Build complete!
echo   Run:  backend\server.exe
echo   Then visit: http://localhost:8080
echo ========================================
pause
