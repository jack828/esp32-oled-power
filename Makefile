SHELL:=/bin/bash
PWD=$(shell pwd)
PORT=/dev/ttyUSB0
FQBN=esp32:esp32:esp32
FILENAME=esp32

compile:
	arduino-cli compile \
		--fqbn $(FQBN):DebugLevel=info \
		--log-level=info \
		--libraries $(PWD)/libraries/ \
		--build-path=$(PWD)/build \
		--build-property 'compiler.warning_level=all' \
		--warnings all \
		.

upload:
	arduino-cli upload \
		-p $(PORT) \
		--fqbn $(FQBN) \
		--log-level=debug \
		--input-dir=$(PWD)/build

monitor:
	arduino-cli monitor \
		-p $(PORT) \
		--config Baudrate=115200

clean:
	rm -rf ./build

