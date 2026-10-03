IN_CONTAINER := $(shell test -f /run/.containerenv && echo 1 || echo 0)
ifeq ($(IN_CONTAINER),1)
  TBX :=
else
  TBX := toolbox run --
endif

PIO := $(TBX) pio

.PHONY: connect-esp32 build clean rebuild reboot upload kill-tio kill-upload reboot-connect setup-permissions setup-toolbox

connect-esp32:
	tio /dev/ttyUSB0 -b 115200

build:
	$(PIO) run -e d1_wroom_02 && cp .pio/build/d1_wroom_02/firmware.bin .pio/build/d1_wroom_02/seed-oven-v3.0.X.bin

clean:
	$(PIO) run -t clean

rebuild: clean build

reboot:
	$(TBX) python3 -c "import serial, time; s = serial.Serial('/dev/ttyUSB0', 115200); s.write(b'r\n'); time.sleep(0.1); s.close()"
	@echo "ESP32 reset command sent."

upload: kill-tio
	$(PIO) run -t upload -e d1_wroom_02 && tio /dev/ttyUSB0 -b 115200

kill-tio:
	-killall -q tio 2>/dev/null || true	

kill-upload: kill-tio upload

reboot-connect: kill-tio reboot connect-esp32

setup-permissions:
	sudo curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules -o /etc/udev/rules.d/99-platformio-udev.rules
	sudo udevadm control --reload-rules
	sudo udevadm trigger
	@echo "PlatformIO udev rules installed. Serial ports are now accessible from host and toolbox."

setup-toolbox:
	$(TBX) sudo dnf install -y python3-pip
	$(TBX) pip install -U platformio
	@echo "Toolbox setup complete with PlatformIO."