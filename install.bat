@echo off
setlocal EnableDelayedExpansion

REM Check if running as administrator
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo ========================================
    echo  Administrator privileges required
    echo ========================================
    echo.
    echo Requesting elevation...
    echo.
    
    REM Use PowerShell method (works on all modern Windows)
    powershell -Command "Start-Process cmd -ArgumentList '/c ""%~f0"" %*' -Verb RunAs" 2>nul
    exit /b
)

echo Installing Modern C++ Matrix Screensaver...

REM Build first if needed
if not exist "%~dp0build\MatrixScreensaver.scr" (
    echo Screensaver not found. Building first...
    
    REM Try to setup VS environment if not already done
    where cl >nul 2>&1
    if %errorlevel% neq 0 (
        echo Setting up Visual Studio environment...
        
        REM Try VS 2022 Community first
        if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
            call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
        ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
            call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
        ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" (
            call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
        ) else (
            echo Visual Studio 2022 not found! Please install Visual Studio 2022 with C++ development tools.
            pause
            exit /b 1
        )
    )
    
    call "%~dp0build.bat"
    if %errorlevel% neq 0 (
        echo Build failed!
        pause
        exit /b 1
    )
)

REM Copy to System32 as .scr file
copy "%~dp0build\MatrixScreensaver.scr" "%SystemRoot%\System32\" /Y

if %errorlevel% neq 0 (
    echo Installation failed!
    pause
    exit /b 1
)

REM Copy mask image if it exists
if exist "%~dp0mask.png" (
    copy "%~dp0mask.png" "%SystemRoot%\System32\"
    echo Copied mask.png to System32
)

if exist "%~dp0mask.jpg" (
    copy "%~dp0mask.jpg" "%SystemRoot%\System32\"
    echo Copied mask.jpg to System32
)

echo.
echo Modern C++ Matrix Screensaver installed successfully!
echo.
echo Features:
echo - DirectX 11 + Direct2D hardware acceleration
echo - C++23 modern implementation
echo - Configurable settings dialog
echo - Mask image support for density control
echo - Multi-monitor support
echo - High performance rendering
echo.
echo You can now select "MatrixScreensaver" from Display Settings ^> Screen Saver
echo.
pause