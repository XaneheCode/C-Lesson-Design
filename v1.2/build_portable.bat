@echo off
setlocal
cd /d "%~dp0"

echo [1/3] Building backend...
gcc -Wall -Wextra -O2 -o backend\server.exe backend\server.c backend\data.c backend\utils.c backend\product.c backend\sale.c backend\purchase.c backend\stats.c -lws2_32
if errorlevel 1 exit /b 1

echo [2/3] Building hidden launcher...
gcc -Wall -Wextra -O2 -mwindows -o portable-launcher.exe launcher.c -lws2_32 -lshell32
if errorlevel 1 exit /b 1

echo [3/3] Creating portable entry point...
powershell -NoProfile -Command "$name=(-join [char[]](0x542F,0x52A8,0x7CFB,0x7EDF))+'.exe'; Copy-Item -Force '.\portable-launcher.exe' (Join-Path '.' $name)"
if errorlevel 1 exit /b 1
del /q portable-launcher.exe

echo Portable folder is ready.
endlocal
