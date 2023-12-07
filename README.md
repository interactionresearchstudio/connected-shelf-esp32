# Connected Shelf ESP32 Firmware

Firmware for the ESP32 to communicate with the IRS IOT server.
The device can be left to post photos at 30 minute intervals or can be configured to be a captive portal to allow users to edit settings such as brightness, contrast, framesize etc.

## Hardware

- ESP32CAM
- Push Button (soldered to GPIO 14 and GND)

## Networking

The ESP32 creates an access point name "CS-12345678" (unique number) and runs a local webpage at the IP `192.168.2.1`
Once updated with WiFi credentials, the camera will connect to server and post every 30 minutes. To change settings while connected to your WiFi network, press the button to disconnect from your network and create the access point.

## How to upload

http://interactionresearchstudio.github.io/connected-shelf-esp32
