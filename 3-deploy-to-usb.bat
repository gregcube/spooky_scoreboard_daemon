@echo off
REM 3-deploy-to-usb.bat
REM Deploys the bootable USB installer from Windows

setlocal EnableDelayedExpansion

echo ============================================================================
echo   Spooky Scoreboard USB Installer Deployment (Windows)
echo ============================================================================
echo.

REM Get the version from VERSION file
set VERSION_FILE=%~dp0VERSION
if not exist "%VERSION_FILE%" (
    echo [ERROR] VERSION file not found: %VERSION_FILE%
    exit /b 1
)

REM Read version from file
set /p VERSION=<%VERSION_FILE%
REM Trim whitespace
for /f "tokens=* delims= " %%a in ("%VERSION%") do set VERSION=%%a

echo [INFO] Version: %VERSION%
echo.

REM Check if installer-output directory exists
set INSTALLER_OUTPUT=%~dp0installer-output
if not exist "%INSTALLER_OUTPUT%" (
    echo [ERROR] installer-output directory not found
    echo [ERROR] Run 2-build-installer.sh first
    exit /b 1
)

REM Check if ZIP file exists
set ZIP_FILE=%INSTALLER_OUTPUT%\ssbd-installer-%VERSION%.zip
if not exist "%ZIP_FILE%" (
    echo [ERROR] Installer ZIP not found: %ZIP_FILE%
    echo [ERROR] Run 2-build-installer.sh first
    exit /b 1
)

REM Check if SHA256 file exists
set SHA_FILE=%INSTALLER_OUTPUT%\ssbd-installer-%VERSION%.sha256
if not exist "%SHA_FILE%" (
    echo [ERROR] SHA256 file not found: %SHA_FILE%
    exit /b 1
)

REM Get file size
for %%A in ("%ZIP_FILE%") do set ZIP_SIZE=%%~zA
set /a ZIP_SIZE_MB=%ZIP_SIZE% / 1048576
set /a ZIP_SIZE_GB=%ZIP_SIZE% / 1073741824
echo [INFO] Found installer: ssbd-installer-%VERSION%.zip (%ZIP_SIZE_MB% MB / %ZIP_SIZE_GB% GB)

REM Check if file is over 4GB (FAT32 limit)
set FAT32_LIMIT=4294967295
if %ZIP_SIZE% GTR %FAT32_LIMIT% (
    echo.
    echo [WARN] ZIP file is larger than 4GB ^(%ZIP_SIZE_GB% GB^)
    echo [WARN] FAT32 file systems cannot store files larger than 4GB
    echo [WARN] Your USB must be formatted as exFAT or NTFS
    echo.
)

echo.

REM Prompt for drive letter
:PROMPT_DRIVE
set /p DRIVE_LETTER="Enter the drive letter of your USB drive (e.g., E, F, G): "

REM Validate and normalize drive letter
set DRIVE_LETTER=%DRIVE_LETTER::=%
set DRIVE_LETTER=%DRIVE_LETTER: =%
set DRIVE_LETTER=%DRIVE_LETTER:~0,1%

if "%DRIVE_LETTER%"=="" (
    echo [ERROR] No drive letter entered
    goto PROMPT_DRIVE
)

REM Convert to uppercase
for %%i in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
    if /i "%DRIVE_LETTER%"=="%%i" set DRIVE_LETTER=%%i
)

REM Check if drive exists
set USB_DRIVE=%DRIVE_LETTER%:
if not exist "%USB_DRIVE%\" (
    echo [ERROR] Drive %USB_DRIVE% does not exist
    echo [ERROR] Make sure the USB drive is connected
    goto PROMPT_DRIVE
)

echo [INFO] Using drive: %USB_DRIVE%
echo.

REM Check filesystem type
for /f "tokens=2 delims=:" %%a in ('fsutil fsinfo volumeinfo %USB_DRIVE% ^| findstr /C:"File System Name"') do set FS_TYPE=%%a
set FS_TYPE=%FS_TYPE: =%
echo [INFO] File system: %FS_TYPE%

REM Recommend FAT32 for bootable USB
if /i not "%FS_TYPE%"=="FAT32" (
    echo [WARN] USB is not formatted as FAT32
    echo [WARN] For maximum compatibility, bootable USB drives should use FAT32
    echo [WARN] If the pinball machine doesn't boot, reformat as FAT32 and try again
    echo.
)

REM Show current USB contents
echo [INFO] Current USB drive contents:
dir "%USB_DRIVE%\" /w
echo.

REM Warning about overwriting
echo [WARN] This will copy the installer to the USB drive root.
echo [WARN] Any existing 'spooky-scoreboard-installer' directory will be overwritten.
echo.
set /p CONFIRM="Continue? [y/N]: "
if /i not "%CONFIRM%"=="y" (
    echo [INFO] Deployment cancelled
    exit /b 0
)

echo.

REM Copy SHA256 to USB for verification
echo [INFO] Copying SHA256 checksum to USB drive...
copy /Y "%SHA_FILE%" "%USB_DRIVE%\" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy SHA256 file
    exit /b 1
)
echo [OK] SHA256 copied to USB

REM Save current directory
pushd "%CD%"

REM Change to USB drive
%USB_DRIVE%
cd \

REM Remove old installer directory if it exists
if exist "spooky-scoreboard-installer" (
    echo [INFO] Removing old installer directory...
    rd /s /q "spooky-scoreboard-installer"
)

REM Extract directly from source (don't copy the large ZIP first)
echo [INFO] Extracting installer to USB drive...
echo [INFO] This may take a few minutes...

REM Use PowerShell to extract
powershell -Command "Expand-Archive -Path '%ZIP_FILE%' -DestinationPath '%USB_DRIVE%\' -Force"
if errorlevel 1 (
    echo [ERROR] Failed to extract ZIP file
    popd
    exit /b 1
)
echo [OK] Installer extracted

REM Verify extraction
if not exist "spooky-scoreboard-installer\boot" (
    echo [ERROR] Extraction failed: boot directory not found
    popd
    exit /b 1
)

REM Navigate to boot directory
cd spooky-scoreboard-installer\boot

echo [INFO] Boot directory: %CD%
echo [INFO] Contents:
dir /b
echo.

REM Check for bootinst.bat
if not exist "bootinst.bat" (
    echo [ERROR] bootinst.bat not found in boot directory
    echo [ERROR] Installer may be corrupted
    popd
    exit /b 1
)

REM Run bootinst.bat as administrator
echo [INFO] Running bootinst.bat to make USB bootable...
echo [INFO] This requires administrator privileges
echo.
echo ============================================================================
echo.

REM Check if we're already running as admin
net session >nul 2>&1
if errorlevel 1 (
    echo [WARN] Not running as administrator
    echo [WARN] Please run bootinst.bat manually as administrator:
    echo.
    echo   1. Open Command Prompt as Administrator
    echo   2. Run: %USB_DRIVE%\spooky-scoreboard-installer\boot\bootinst.bat
    echo.
) else (
    REM We're admin, run bootinst.bat
    call bootinst.bat
    if errorlevel 1 (
        echo [ERROR] bootinst.bat failed
        popd
        exit /b 1
    )
)

echo.
echo ============================================================================
echo [OK] USB Installer Deployment Complete
echo ============================================================================
echo.
echo [INFO] USB drive at %USB_DRIVE% should now be bootable
echo [INFO] You can safely eject the drive and use it to install SSBD
echo.

REM Return to original directory
popd

echo.
echo [OK] Done!
echo.

endlocal
