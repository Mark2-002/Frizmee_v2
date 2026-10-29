# Frizmee_v1

A Zephyr-based Nordic nRF54L15 firmware project for a motion-tracking wearable device built around the Bosch BNO055 inertial sensor. The device monitors acceleration, gyroscope, magnetometer, quaternion data, and orientation, then transmits the values over BLE while also supporting wireless charging, NFC, LED control, and BLE-based firmware update capability.

## Overview

Frizmee_v1 is designed around a BLE-enabled sensor node that:

- tracks motion and orientation using the BNO055 IMU
- reads battery level and state of charge
- streams sensor data over Bluetooth
- supports BLE-controlled RGB LED output
- includes NFC tag support for lightweight interaction
- uses Nordic low-power/system-off handling to reduce power consumption
- supports BLE FOTA (Firmware Over The Air) via SMP server support

This repository is organized as a Zephyr application, with the main firmware living in `src/main.c` and the BLE logic separated into `src/ble.h`.

## Key Features

- BNO055 sensor acquisition
  - acceleration
  - gyroscope
  - magnetometer
  - quaternion
  - Euler angles
  - linear acceleration
- BLE communication and notifications
- LED control through BLE payloads
- NFC tag emulation using NDEF URI payloads
- Wireless charging support / battery monitoring
- System-off and low-power behavior
- watchdog support and reset handling
- SMP server/FOTA readiness for BLE firmware updates

## Hardware Target

This project targets Nordic nRF-based hardware, especially the nRF54L15 platform. The repository contains:

- `nrf54l15dk_nrf54l15_cpuapp.overlay` for board-specific configuration
- `boards/` directory for hardware-related definitions
- Zephyr configuration files (`prj.conf`, `prj-coex.conf`, `sysbuild.conf`)

## Repository Structure

```text
Frizmee_v1/
├── CMakeLists.txt
├── Kconfig
├── boards/
├── nrf54l15dk_nrf54l15_cpuapp.overlay
├── prj.conf
├── prj-coex.conf
├── sample.yaml
├── sysbuild.conf
└── src/
    ├── ble.h
    ├── main.c
    ├── sys_off.h
    └── ws2812.h
```

## Core Firmware Behavior

The application initializes the device, configures watchdogs, sets up BLE, configures the BNO055 sensor, and then runs a main loop that:

1. polls and reads battery voltage / charge
2. reads IMU values from the BNO055
3. packages the data for BLE transmission
4. sends notifications when connected
5. controls the WS2812 RGB LED according to command or state
6. handles low-power / system-off logic when the device is inactive

## BLE Functionality

The project includes Bluetooth initialization and advertising through the custom BLE layer in `src/ble.h`. It supports:

- BLE advertising
- connection state tracking
- notify/stream of movement-related data
- LED control by BLE payload
- BLE FOTA/SMP services

## NFC Support

NFC functionality is implemented in the main firmware using the NDEF URI message flow with a tag URL (`mlworkx.com`). The code sets up NFC T2T emulation and starts the tag when enabled.

## Build Instructions

This project uses the Zephyr RTOS and the Nordic nRF Connect SDK toolchain.

### Prerequisites

- nRF Connect SDK / Zephyr environment
- `west` tool
- CMake, Ninja, and a suitable GNU ARM toolchain
- nRF development board or compatible target

### Example build

```bash
west build -p always -b nrf54l15dk/nrf54l15/cpuapp .
```

### Flash to board

```bash
west flash
```

If you are using a custom build setup, ensure the Zephyr base and toolchain environment variables are configured correctly before compiling.

## Configuration Notes

- `prj.conf` contains the default Zephyr app configuration.
- `prj-coex.conf` appears to be a coexistence-specific configuration.
- `sysbuild.conf` is included for multi-image / system build settings.
- `sample.yaml` includes sample metadata and test definitions for Zephyr-based validation.

## Important Notes

- The firmware is designed around Nordic hardware and Zephyr drivers.
- The code is written in C and uses direct device register access for the BNO055 and Nordic peripherals.
- This repository appears to be an experimental/prototype wearable device project, with features intended for BLE monitoring, control, and firmware update.

## License

No explicit license file was found in the repository. If this project is intended for public reuse, adding an open-source license is recommended.

## Suggested Next Improvements

- add a project-specific architecture diagram
- document the BLE GATT service structure
- document NFC payload format and expected application behavior
- add a clear build and flashing guide for the exact hardware revision
- add a license file and contribution instructions

## Contact / Project Context

This project appears to be a personal/prototype embedded system for motion tracking and BLE interaction. The repository description indicates a focus on:

- motion tracking with BNO055
- NFC interaction
- wireless charging
- BLE-control of LEDs and sensor values
- firmware updates over BLE (SMP server)

