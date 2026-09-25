@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Unreal Projects\OvalOffice\Scripts\LaunchFourYears.ps1"
if errorlevel 1 pause
