PORT=/dev/ttyACM0
BOARD=esp32:esp32:pico32
SKETCH=friendship-lamps-v2.ino

.PHONY: compile
compile:
	arduino-cli compile --fqbn ${BOARD} ${SKETCH}

.PHONY: upload
upload:
	arduino-cli upload -p ${PORT} --fqbn ${BOARD} ${SKETCH}
