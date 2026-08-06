@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ==========================================
echo  N_m3u8DL-RE GUI Qt - Build Script
echo ==========================================
echo.

:: =============================================
:: Configurable paths
:: =============================================
set QT_VER=6.11.1
set QT_KIT=llvm-mingw_64
set QT_DIR=D:\Qt\%QT_VER%\%QT_KIT%
set TOOLCHAIN=D:\Qt\Tools\llvm-mingw1706_64

:: =============================================
:: Detect Qt version if specified path missing
:: =============================================
if exist "%QT_DIR%" goto :qt_ok

echo [WARNING] Qt not found at: %QT_DIR%
echo Trying to auto-detect Qt installation...
set QT_VER=
for /d %%d in (D:\Qt\*.*.*) do (
    if exist "%%d\llvm-mingw_64\bin\Qt6Core.dll" (
        set QT_VER=%%~nxd
        set QT_DIR=%%d\llvm-mingw_64
    )
)
if defined QT_VER goto :qt_ok
echo [ERROR] Could not find Qt llvm-mingw_64 installation under D:\Qt
echo Please set QT_DIR manually in build.bat
pause
exit /b 1
:qt_ok

:: =============================================
:: Detect toolchain
:: =============================================
if exist "%TOOLCHAIN%\bin\x86_64-w64-mingw32-g++.exe" goto :tc_ok

echo [WARNING] LLVM-MinGW toolchain not found at: %TOOLCHAIN%
echo Trying to auto-detect...
set TOOLCHAIN=
for /d %%d in (D:\Qt\Tools\llvm-mingw*) do (
    if exist "%%d\bin\x86_64-w64-mingw32-g++.exe" (
        set TOOLCHAIN=%%d
    )
)
if defined TOOLCHAIN goto :tc_ok
echo [ERROR] Could not find LLVM-MinGW toolchain under D:\Qt\Tools
echo Please install llvm-mingw or set TOOLCHAIN manually in build.bat
pause
exit /b 1
:tc_ok

echo Qt:       %QT_DIR%
echo Toolchain: %TOOLCHAIN%
echo.

:: =============================================
:: Set PATH (save original for later)
:: =============================================
set ORIGINAL_PATH=%PATH%
set PATH=%TOOLCHAIN%\bin;%QT_DIR%\bin;%PATH%

:: =============================================
:: Step 1: CMake Configure
:: =============================================
echo [1/2] Configuring CMake...

cmake -G "MinGW Makefiles" ^
    -S . ^
    -B build ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_C_COMPILER="%TOOLCHAIN%\bin\x86_64-w64-mingw32-gcc.exe" ^
    -DCMAKE_CXX_COMPILER="%TOOLCHAIN%\bin\x86_64-w64-mingw32-g++.exe" ^
    -DCMAKE_RC_COMPILER="%TOOLCHAIN%\bin\x86_64-w64-mingw32-windres.exe"

if errorlevel 1 (
    echo.
    echo [ERROR] CMake configure failed!
    pause
    exit /b 1
)

echo.
echo Configure done.
echo.

:: =============================================
:: Step 2: Build
:: =============================================
echo [2/2] Building Release...

cmake --build build --config Release

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo ==========================================
echo  Build completed successfully!
echo  Output: build\N_m3u8DL_RE_GUI_Qt.exe
echo ==========================================
echo.

:: =============================================
:: Step 3: Launch
:: =============================================
set EXE_PATH=%~dp0build\N_m3u8DL_RE_GUI_Qt.exe

if not exist "%EXE_PATH%" (
    echo [ERROR] Executable not found: %EXE_PATH%
    pause
    exit /b 1
)

echo Starting application...
:: Restore original PATH so exe loads DLLs from its own directory, not Qt bin
set PATH=%ORIGINAL_PATH%
start "N_m3u8DL-RE GUI" /D "%~dp0build" "%EXE_PATH%"
echo Application launched. Closing terminal...
echo.
:: Success: auto-close the terminal window
exit /b 0
