# Co2-Sensor-Software

This repository is the Software part of the Hardware project https://github.com/twam/Co2-Sensor-Hardware.

## Usage
Connecting the CO2 Sensor via USB you get log access via USB Serial.

## Configuration
You can enter network configuration mode by pressing Button-1 (leftmost) while powering on. It starts an open Wifi AP with the SSID "Co2-Sensor". 

The initial configuration UI is available at http://192.168.4.1/config. You are able to enter SSID and Password of an already existing Wifi network to connect to.

After configuration the device connects to the Wifi network specified and is reachable with the provided hostname at http://<hostname> .

## Updating
Use the [PlatformIO](https://platformio.org) IDE to download dependencies, tools and compiling.

In order to flash, you need to have  the CO2 Sensor connected to your build system via USB.

## Reset

Pressing the left two buttons will trigger a hardware reset/reboot.