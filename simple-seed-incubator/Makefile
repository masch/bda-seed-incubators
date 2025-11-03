.PHONY: connect-esp32 build clean rebuild upload kill-tio kill-upload

connect-esp32:
	tio /dev/ttyUSB0 -b 115200

build:
	pio run -e nodemcu-32s && cp .pio/build/nodemcu-32s/firmware.bin .pio/build/nodemcu-32s/simple-seed-incubator-v3.0.X.bin

clean:
	pio run -t clean

rebuild: clean build

upload:
	killall tio & pio run -t upload -e nodemcu-32s && tio /dev/ttyUSB0 -b 115200

kill-tio:
	killall tio	

kill-upload: kill-tio upload