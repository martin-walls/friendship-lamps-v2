set port=COM3
set board=esp32:esp32:pico32
set sketch=friendship-lamps-v2.ino

arduino-cli compile --fqbn %board% %sketch%
arduino-cli upload -p %port% --fqbn %board% %sketch%
