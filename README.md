<div align="center">
  <a href="https://bosquesdeagua.ar/">
    <img src="images/cropped-logo-blanco-big.webp" alt="logo" width="500"/>
  </a>
  </br>
  </br>
</div>

# Seed Incubators and Oven Controller

This repository contains the source code for three projects to control seed incubators and a seed oven using the ESP32 microcontroller.

## Projects

- **Simple Seed Incubator**: Controls a single seed incubator with a DHT22 temperature sensor.
- **Double Seed Incubator**: Controls a double seed incubator with two DHT22 temperature sensors, one for each chamber.
- **Seed Oven**: Controls a seed oven with a MAX31865 temperature sensor.

## Features

- **Remote Control**: The incubators and oven can be controlled remotely via a web server. The server can be used to set the desired temperature and to monitor the current temperature.
- **Offline Mode**: If the WiFi connection is lost, the devices will continue to operate in offline mode, using the last set temperature or a default temperature.
- **OTA Firmware Updates**: The firmware of the devices can be updated over-the-air (OTA) using Firebase.
- **Temperature Control**: The devices use a PID control loop to maintain the desired temperature.

## Hardware Specifications

All devices across this project standardize on the same microcontroller module:

- **Microcontroller**: **Espressif ESP32-WROOM-32E**
  - **CPU**: Xtensa® dual-core 32-bit LX6 microprocessor, up to 240 MHz
  - **Memory**: 520 KB SRAM, 448 KB ROM, 4 MB SPI Flash
  - **Wireless**: 2.4 GHz Wi-Fi (802.11 b/g/n) + Bluetooth® 4.2 / BLE
  - **Operating System**: Native FreeRTOS multitasking

### Device Hardware Matrix

| Device | Microcontroller | Temperature Sensor | Actuators |
| :--- | :--- | :--- | :--- |
| **Simple Seed Incubator** | ESP32-WROOM-32E | DHT22 (Digital AM2302) | Relays (Heater, Cooler, Fan) |
| **Double Seed Incubator** | ESP32-WROOM-32E | 2x DHT22 (Dual chamber) | Relays (Dual Heater, Cooler, Fan) |
| **Seed Oven** | ESP32-WROOM-32E | MAX31865 (Precision RTD PT100/PT1000) | Relays (Heating elements) |

## Software & Tooling

- **PlatformIO**: Embedded build system supporting local and Fedora Silverblue / Toolbox container environments.
- **Arduino Framework & FreeRTOS**: Multi-tasking application stack running networking and control loops on separate cores/tasks.
- **Unified Makefiles**: Standardized targets across all projects:
  - `make build`: Compile firmware into `.bin`.
  - `make upload`: Upload firmware to ESP32 over serial.
  - `make reboot`: Software reboot via serial command (`r`).
  - `make reboot-connect`: Reboot and open serial monitor (`tio`).
  - `make setup-permissions`: Install official PlatformIO udev rules for serial access.
  - `make setup-toolbox`: Provision Toolbox container dependencies automatically.

## Getting Started

1.  Clone this repository.
2.  Open one of the project folders in PlatformIO.
3.  Create an `env.h` file in the `src` directory of the project with the following content:

```c++
#define SSID "your_wifi_ssid"
#define WIFIPASS "your_wifi_password"
#define API_KEY "your_firebase_api_key"
#define USER_EMAIL "your_firebase_user_email"
#define USER_PASSWORD "your_firebase_user_password"
#define STORAGE_BUCKET_ID "your_firebase_storage_bucket_id"
#define FIRMWARE_PATH "firmware.bin"
#define FIRMWARE_VERSION "1.0.0"
```

4.  Build and upload the firmware to your ESP32 device.
