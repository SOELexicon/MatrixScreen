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
    
    REM Try PowerShell method first (more reliable on modern Windows)
    powershell -Command "Start-Process cmd -ArgumentList '/c ""%~f0"" %*' -Verb RunAs" 2>nul
    if %errorlevel% equ 0 (
        exit /b
    )
    
    REM Fallback to VBScript method if PowerShell fails
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "cmd.exe", "/c ""%~f0"" %*", "", "runas", 1 >> "%temp%\getadmin.vbs"
    
    "%temp%\getadmin.vbs"
    del "%temp%\getadmin.vbs"
    
    exit /b
)

echo Installing Modern C++ Matrix Screensaver...

REM Build first if needed
if not exist "%~dp0build\MatrixScreensaver.scr" (
    echo Screensaver not found. Building first...
    call "%~dp0build.bat"
    if %errorlevel% neq 0 (
        echo Build failed!
        pause
        exit /b 1
    )
)

REM Copy to System32 as .scr file
copy "%~dp0build\MatrixScreensaver.scr" "%SystemRoot%\System32\"

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