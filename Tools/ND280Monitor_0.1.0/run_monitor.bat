@echo off
setlocal
cd /d "%~dp0"
py -m pip install -r requirements.txt
py -m nd280_monitor.main
pause
