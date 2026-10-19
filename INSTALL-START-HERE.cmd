@echo off
setlocal
echo Extract the whole ZIP before running this installer.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-HauntCue.ps1"
echo.
pause
