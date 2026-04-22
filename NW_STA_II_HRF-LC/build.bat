@echo off
REM Build script for Windows
REM This script provides a convenient way to build the project on Windows
REM Uses MSYS2 toolchain from D:\MSYS2 (not in system PATH)

REM Sync revision information first
call version\SyncRevision.bat

setlocal

echo ==========================================
echo BPLC_NW_STA Build Script for Windows
echo ==========================================
echo.

REM MSYS2配置
set MSYS2_ROOT=D:\MSYS2
set MSYS2_BIN=%MSYS2_ROOT%\usr\bin
set MSYS2_MINGW64=%MSYS2_ROOT%\mingw64\bin

REM 将MSYS2路径添加到当前会话的PATH（仅临时）
set PATH=%MSYS2_MINGW64%;%MSYS2_BIN%;%PATH%

echo 使用MSYS2工具链: %MSYS2_ROOT%
echo.

REM Check if make is available
where make >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: 'make' 命令未找到！
    echo.
    echo 请检查MSYS2安装路径是否正确: %MSYS2_ROOT%
    echo 确保以下文件存在:
    echo   - %MSYS2_BIN%\make.exe
    echo.
    echo 如果MSYS2未安装make，请在MSYS2终端中运行:
    echo   pacman -S make
    echo.
    pause
    exit /b 1
)

REM Check if ARM toolchain is available
where arm-none-eabi-gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: ARM工具链未找到！
    echo.
    echo 请在MSYS2中安装ARM工具链:
    echo   pacman -S mingw-w64-x86_64-arm-none-eabi-gcc
    echo   pacman -S mingw-w64-x86_64-arm-none-eabi-binutils
    echo   pacman -S mingw-w64-x86_64-arm-none-eabi-newlib
    echo.
    echo 或者从以下地址下载并安装到MSYS2:
    echo https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
    echo.
    pause
    exit /b 1
)

echo ARM工具链已找到:
arm-none-eabi-gcc --version | findstr "gcc"
echo.
echo Make工具:
make --version | findstr "GNU Make"
echo.

REM Parse command line arguments
if "%1"=="" goto build
if "%1"=="clean" goto clean
if "%1"=="with-prebuilt-libs" goto prebuilt
if "%1"=="libs" goto libs_only
if "%1"=="help" goto help
if "%1"=="-h" goto help
if "%1"=="--help" goto help

:build
echo Building project with library compilation...
echo.
make %*
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build FAILED!
    pause
    exit /b 1
)
echo.
echo Build completed successfully!
echo.
echo Output files:
echo   Libraries (gnu\lib\):
echo     - libmac.a (MAC layer library)
echo     - libphy.a (PHY layer library)
echo.
echo   Binaries (gnu\bin\):
echo     - OS3.elf (ELF executable with debug info)
echo     - OS3.hex (Intel HEX format for flashing)
echo     - OS3.bin (Binary format for flashing)
echo.
goto end

:prebuilt
echo Building project using prebuilt libraries...
echo.
echo NOTE: This build mode uses precompiled MAC and PHY libraries.
echo       Library source code is NOT required for this build.
echo.
make with-prebuilt-libs
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build FAILED!
    echo.
    echo Make sure the following library files exist:
    echo   - gnu\lib\libmac.a
    echo   - gnu\lib\libphy.a
    echo.
    pause
    exit /b 1
)
echo.
echo Build completed successfully using prebuilt libraries!
echo.
echo Output files:
echo   Binaries (gnu\bin\):
echo     - OS3.elf (ELF executable with debug info)
echo     - OS3.hex (Intel HEX format for flashing)
echo     - OS3.bin (Binary format for flashing)
echo.
goto end

:libs_only
echo Building libraries only...
echo.
make libs
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Library build FAILED!
    pause
    exit /b 1
)
echo.
echo Libraries built successfully!
echo.
echo Output files (gnu\lib\):
echo   - libmac.a (MAC layer library)
echo   - libphy.a (PHY layer library)
echo.
goto end

:clean
echo Cleaning build files...
make clean
echo.
echo Clean completed!
goto end

:help
echo Usage: build.bat [target] [options]
echo.
echo Available targets:
echo   (none)              - Build the project with library compilation (default)
echo   with-prebuilt-libs  - Build using prebuilt MAC/PHY libraries (for outsourcing)
echo   libs                - Build only MAC and PHY libraries
echo   clean               - Clean build files
echo   help                - Show this help message
echo.
echo Examples:
echo   build.bat                    - Full build with library compilation
echo   build.bat with-prebuilt-libs - Build using existing libraries (no lib source needed)
echo   build.bat libs               - Build only the libraries
echo   build.bat clean              - Clean build files
echo   build.bat -j4                - Build with 4 parallel jobs
echo.
echo For outsourcing workflow:
echo   1. Run 'build.bat libs' to create libmac.a and libphy.a
echo   2. Provide gnu\lib\libmac.a and gnu\lib\libphy.a to external developers
echo   3. External developers use 'build.bat with-prebuilt-libs' to build
echo      (they don't need MAC/PHY source code)
echo.
goto end

:end
endlocal