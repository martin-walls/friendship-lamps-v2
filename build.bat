@echo off

set port=COM3
set board=esp32:esp32:pico32
set sketch=friendship-lamps-v2.ino

arduino-cli compile --fqbn %board% %sketch%
if %ERRORLEVEL% GEQ 1 EXIT /B 1
arduino-cli upload -p %port% --fqbn %board% %sketch%
