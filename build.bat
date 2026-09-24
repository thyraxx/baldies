@echo off
setlocal EnableDelayedExpansion

echo ======================================================
echo           BALDIES C ENGINE - BUILD PIPELINE
echo ======================================================
echo.

set "VCVARS="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
)

if "%VCVARS%"=="" (
    echo [ERROR] Could not find Visual Studio vcvarsall.bat!
    exit /b 1
)

echo [1/3] Initializing MSVC x64 build environment...
call "%VCVARS%" x64 >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to initialize MSVC x64!
    exit /b %ERRORLEVEL%
)

if not exist bin mkdir bin
if not exist obj mkdir obj

set "CFLAGS=/nologo /O2 /W3 /MD /utf-8 /D_CRT_SECURE_NO_WARNINGS /Isrc"
set "LIBS=gdi32.lib user32.lib winmm.lib shell32.lib"

set "ASSET_SRCS=src\assets\asset_path.c src\assets\bal_palette.c src\assets\bal_tiles.c src\assets\bal_map.c src\assets\bal_sprites.c src\assets\bal_sfx.c src\assets\bal_midi.c"
set "RENDER_SRCS=src\render\surface.c src\render\map_renderer.c"
set "GAME_SRCS=src\game\camera.c src\game\hud.c src\game\menu.c src\game\entities.c src\game\house.c src\game\game_loop.c"
set "PLATFORM_SRCS=src\platform\platform_win32.c"


echo [2/3] Compiling Baldies C Unit Tests (bin\baldies_test.exe)...
cl.exe %CFLAGS% /Fe:bin\baldies_test.exe /Fo:obj\ src\test_main.c %ASSET_SRCS% %RENDER_SRCS% /link %LIBS%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Unit test compilation failed!
    exit /b %ERRORLEVEL%
)

echo [3/3] Compiling Baldies C Game Engine (bin\baldies_c.exe)...
cl.exe %CFLAGS% /Fe:bin\baldies_c.exe /Fo:obj\ src\main.c %ASSET_SRCS% %RENDER_SRCS% %GAME_SRCS% %PLATFORM_SRCS% /link %LIBS%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Game engine compilation failed!
    exit /b %ERRORLEVEL%
)

copy /y bin\baldies_c.exe baldies_c.exe >nul 2>&1
copy /y bin\baldies_test.exe baldies_test.exe >nul 2>&1

echo.
echo ======================================================
echo [SUCCESS] Compilation complete!
echo   - Unit Tests: baldies_test.exe (and bin\baldies_test.exe)
echo   - Game Engine: baldies_c.exe    (and bin\baldies_c.exe)
echo ======================================================
exit /b 0
