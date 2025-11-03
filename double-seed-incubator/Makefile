.PHONY: connect-esp32 build clean rebuild upload kill-tio kill-upload

connect-esp32:
	tio /dev/ttyUSB0 -b 115200

build:
	pio run -e nodemcu && cp .pio/build/nodemcu/firmware.bin .pio/build/nodemcu/double-seed-incubator-v3.0.X.bin

clean:
	pio run -t clean

rebuild: clean build

upload:
	killall tio || pio run -t upload -e nodemcu

kill-tio:
	killall tio	

kill-upload: kill-tio upload