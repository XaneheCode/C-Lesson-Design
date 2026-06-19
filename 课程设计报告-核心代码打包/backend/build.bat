@echo off
chcp 65001 >nul
echo ========================================
echo   文具店销售管理系统 - 编译脚本
echo ========================================

cd /d "%~dp0"

echo.
echo [1/3] 编译数据生成器...
gcc -Wall -O2 -o generate_data.exe generate_data.c
if errorlevel 1 (
    echo 编译数据生成器失败!
    pause
    exit /b 1
)
echo      成功!

echo.
echo [2/3] 生成模拟数据...
cd ..
backend\generate_data.exe
cd backend
if errorlevel 1 (
    echo 生成数据失败!
    pause
    exit /b 1
)
echo      成功!

echo.
echo [3/3] 编译服务器...
gcc -Wall -O2 -o server.exe server.c data.c utils.c product.c sale.c purchase.c stats.c assistant.c -lws2_32 -lwinhttp
if errorlevel 1 (
    echo 编译服务器失败!
    pause
    exit /b 1
)
echo      成功!

echo.
echo ========================================
echo   编译完成!
echo   运行: cd .. && backend\server.exe
echo   然后访问: http://localhost:8080
echo ========================================
pause
