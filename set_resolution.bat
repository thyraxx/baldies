@echo off
setlocal EnableDelayedExpansion

:: Check if Python is available to run the enhanced Python menu
where python >nul 2>nul
if %ERRORLEVEL% equ 0 (
    python "%~dp0set_resolution.py" %*
    exit /b %ERRORLEVEL%
)

:: Fallback native batch menu if Python is not installed
title Baldies Resolution Selector

:menu
cls
echo ======================================================
echo           BALDIES - RESOLUTION SELECTOR
echo ======================================================
echo.
echo Select a display resolution preset:
echo.
echo   [1]  1024 x 768   - Classic 4:3 Window
echo   [2]  1280 x 960   - Standard HD 4:3 Window (Default)
echo   [3]  1440 x 1080  - Full-Height 1080p 4:3 Window
echo   [4]  1600 x 1200  - UXGA 4:3 Window (2.5x Integer Scale)
echo   [5]  1920 x 1440  - QHD 4:3 Window (3x Integer Scale)
echo   [6]  1920 x 1080  - Borderless Fullscreen (1080p)
echo   [7]  2560 x 1440  - Borderless Fullscreen (1440p)
echo   [8]  3840 x 2160  - Borderless Fullscreen (4K UHD)
echo.
echo   [L]  Launch Baldies now
echo   [Q]  Quit
echo.
echo ======================================================
set /p choice="Enter your choice [1-8, L, Q]: "

if "%choice%"=="1" call :apply_res 1024 768 false
if "%choice%"=="2" call :apply_res 1280 960 false
if "%choice%"=="3" call :apply_res 1440 1080 false
if "%choice%"=="4" call :apply_res 1600 1200 false
if "%choice%"=="5" call :apply_res 1920 1440 false
if "%choice%"=="6" call :apply_res 1920 1080 true
if "%choice%"=="7" call :apply_res 2560 1440 true
if "%choice%"=="8" call :apply_res 3840 2160 true
if /i "%choice%"=="L" goto launch
if /i "%choice%"=="Q" goto end
goto post_apply

:apply_res
set W=%1
set H=%2
set FS=%3

powershell -NoProfile -Command "$f='ddraw.ini'; $c=[System.IO.File]::ReadAllText($f); $c=[regex]::new('(?m)^width=\d*').Replace($c,'width=%W%',1); $c=[regex]::new('(?m)^height=\d*').Replace($c,'height=%H%',1); $c=[regex]::new('(?m)^fullscreen=(true|false)').Replace($c,'fullscreen=%FS%',1); [System.IO.File]::WriteAllText($f,$c)"

echo.
echo [OK] Resolution set to %W% x %H% (Fullscreen: %FS%)
goto :eof

:post_apply
echo.
set /p launch_now="Launch Baldies now? (Y/N): "
if /i "%launch_now%"=="Y" goto launch
goto menu

:launch
echo Launching Baldies...
start "" "%~dp0baldies.exe"
goto end

:end
exit /b 0
