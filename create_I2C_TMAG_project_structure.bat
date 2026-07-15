@echo off
setlocal

REM Create project folder structure for the I2C-TMAG Sensor Array project
REM Run this .bat from the directory where you want the project folder created.

set PROJECT=I2C-TMAG-Sensor-Array

echo Creating project structure: %PROJECT%

mkdir "%PROJECT%" 2>nul
mkdir "%PROJECT%\Hardware" 2>nul
mkdir "%PROJECT%\Hardware\KiCad" 2>nul
mkdir "%PROJECT%\Hardware\PDFs" 2>nul
mkdir "%PROJECT%\Hardware\Photos" 2>nul
mkdir "%PROJECT%\Hardware\Datasheets" 2>nul

mkdir "%PROJECT%\Firmware" 2>nul
mkdir "%PROJECT%\Firmware\BringUp" 2>nul
mkdir "%PROJECT%\Firmware\BringUp\01_Blink" 2>nul
mkdir "%PROJECT%\Firmware\BringUp\02_I2CScanner" 2>nul
mkdir "%PROJECT%\Firmware\BringUp\03_TMAG_ID" 2>nul
mkdir "%PROJECT%\Firmware\BringUp\04_TMAG_XYZ" 2>nul
mkdir "%PROJECT%\Firmware\BringUp\05_SHTC3" 2>nul
mkdir "%PROJECT%\Firmware\BringUp\06_Buzzer" 2>nul
mkdir "%PROJECT%\Firmware\Libraries" 2>nul
mkdir "%PROJECT%\Firmware\Libraries\TMAG5273" 2>nul
mkdir "%PROJECT%\Firmware\Libraries\SHTC3" 2>nul
mkdir "%PROJECT%\Firmware\Application" 2>nul
mkdir "%PROJECT%\Firmware\Application\SensorArray" 2>nul

mkdir "%PROJECT%\Documentation" 2>nul
mkdir "%PROJECT%\Documentation\BringUp_Logs" 2>nul
mkdir "%PROJECT%\Documentation\Notes" 2>nul

mkdir "%PROJECT%\Tools" 2>nul
mkdir "%PROJECT%\TestData" 2>nul

REM Create placeholder files
copy nul "%PROJECT%\README.md" >nul
copy nul "%PROJECT%\Documentation\Bringup.md" >nul
copy nul "%PROJECT%\Documentation\TODO.md" >nul
copy nul "%PROJECT%\Documentation\RegisterMap.md" >nul
copy nul "%PROJECT%\Documentation\BringUp_Logs\000_initial_board_state.md" >nul

REM Add starter text
(
  echo # I2C-TMAG Sensor Array
  echo.
  echo Project for ATmega328PB-based TMAG5273 magnetic sensor bring-up and future sensor array development.
) > "%PROJECT%\README.md"

(
  echo # Initial Board State
  echo.
  echo Date:
  echo Board revision:
  echo Power rail measured:
  echo Programmer used: AVRISP mkII
  echo MCU: ATmega328PB
  echo Clock: internal RC oscillator
  echo Notes:
) > "%PROJECT%\Documentation\BringUp_Logs\000_initial_board_state.md"

echo.
echo Done.
echo Project created at: %CD%\%PROJECT%
pause
