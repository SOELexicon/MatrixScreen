@echo off
echo Building Modern C++ Matrix Screensaver...

REM Check if Visual Studio is available
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo Visual Studio environment not detected. Setting up...
    
    REM Try to find and setup VS 2022
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
    ) else (
        echo Visual Studio 2022 not found!
        echo Please install Visual Studio 2022 with C++ development tools,
        echo or run this script from a Developer Command Prompt.
        pause
        exit /b 1
    )
    
    REM Check again after setup
    where cl >nul 2>&1
    if %errorlevel% neq 0 (
        echo Failed to setup Visual Studio environment.
        echo Please run from Developer Command Prompt.
        pause
        exit /b 1
    )
)

REM Clean previous build completely
if exist build rmdir /s /q build
if exist CMakeCache.txt del CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles

mkdir build
cd build

REM Configure with CMake - specify no PkgConfig needed
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

REM Build the project
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

echo Build successful!
echo.

REM Copy mask image if it exists
cd ..
if exist "mask.png" (
    copy "mask.png" "build\Release\"
    echo Copied mask.png to output directory
)
if exist "mask.jpg" (
    copy "mask.jpg" "build\Release\"
    echo Copied mask.jpg to output directory
)

echo.
echo Matrix Screensaver built successfully!
echo Output: build\Release\MatrixScreensaver.scr
echo.
echo To install:
echo 1. Run install.bat as Administrator
echo 2. Or copy MatrixScreensaver.scr to Windows\System32
echo.
pause