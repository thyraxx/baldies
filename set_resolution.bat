@echo off
setlocal EnableDelayedExpansion

:: Prefer Python if available
where python >nul 2>nul
if %ERRORLEVEL% equ 0 (
    python "%~dp0set_resolution.py" %*
    exit /b %ERRORLEVEL%
)

:: Seamless fallback to native PowerShell script (always present on Windows 10/11)
where powershell >nul 2>nul
if %ERRORLEVEL% equ 0 (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0set_resolution.ps1" %*
    exit /b %ERRORLEVEL%
)

echo [Error] Neither Python nor PowerShell was found on this system.
pause
exit /b 1
