# ESP32 IoT with HTU21d and MQTT

A low-power IoT sensing device using ESP32 + HTU21D. Supports to display sensory data on an OLCD screen and submit to MQTT.

## Requirements

- Install Arduino IDE.
- Install ESP32 board support (https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md).
- Install Adafruit MQTT Library in the Arduino library manager.


## Hardware

I am using WiFi Kit 32, a ESP32 board from HELTEC. Any other ESP32 boards should also work.

The HTU21D and OLED are connected via IIC.  


## Usage

Simply open the Arduino project. Change the parameters at the beginning and download the firmware to the ESP32 board.


## Credit

- HTU21D driver is from Sparkfun.
- OLED driver is from ThingPulse.
- MQTT driver is from Adafruit.
