.PHONY: connect-esp32 build clean rebuild upload kill-tio kill-upload

connect-esp32:
	tio /dev/ttyUSB0 -b 115200

build:
	pio run -e d1_wroom_02 && cp .pio/build/d1_wroom_02/firmware.bin .pio/build/d1_wroom_02/simple-seed-incubator-v3.0.X.bin

clean:
	pio run -t clean

rebuild: clean build

upload:
	killall tio & pio run -t upload -e d1_wroom_02 && tio /dev/ttyUSB0 -b 115200

kill-tio:
	killall tio	

kill-upload: kill-tio upload