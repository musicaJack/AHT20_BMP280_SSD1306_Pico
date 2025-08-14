@echo off
echo ========================================
echo RP2040 AHT20 + BMP280 Sensor Project Build
echo ========================================

REM Set Pico SDK path
set PICO_SDK_PATH=C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\pico-sdk

REM Check if Pico SDK exists
if not exist "%PICO_SDK_PATH%" (
    echo Error: Pico SDK path not found: %PICO_SDK_PATH%
    echo Please make sure Pico SDK is properly installed
    pause
    exit /b 1
)

echo Pico SDK path: %PICO_SDK_PATH%

REM Clean old build files
if exist build (
    echo Cleaning old build files...
    rmdir /s /q build
)

REM Create build directory
echo Creating build directory...
mkdir build
cd build

REM Configure CMake project
echo Configuring CMake project...
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..

REM Check if CMake configuration was successful
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build project
echo Building project...
ninja

REM Check if build was successful
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful!
echo ========================================
echo Generated files:
dir *.uf2
echo.
echo Please copy the generated .uf2 file to RP2040 device for flashing
echo.

REM Return to parent directory
cd ..\..

