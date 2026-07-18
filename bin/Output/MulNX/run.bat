@echo off
cd /d "%~dp0"

set "EXE_PATH=CS2Injector\CS2Injector.exe"

if exist "%EXE_PATH%" (
    start "" "%EXE_PATH%"
) else (
    echo [错误] 找不到文件：%EXE_PATH%
    pause
)