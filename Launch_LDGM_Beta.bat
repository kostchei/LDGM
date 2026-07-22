@echo off
title Legally Distinct Gorka Morka - Shareable Beta
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\launch-beta.ps1"
pause
