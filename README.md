# Connected Shelf ESP32 Firmware

Firmware for the ESP32 to communicate with the IRS IOT server.
The device can be left to post photos at intervals or can be configured to be a captive portal to allow users to edit settings such as brightness, contrast, framesize etc.

## Hardware

- ESP32CAM
- Push Button (soldered to GPIO 14 and GND)

## Networking

The ESP32 creates an access point name "CONNECTED SHELF" and runs a captive portal at the IP `192.168.2.1`
Once updated with WiFi credentials, the camera will connect to server and post every 30 minutes. To change settings while connected to your WiFi network, press the button to disconnect from your network and create the access point.

## How to upload

1. Install Platform IO.
2. If needed, modify upload and port settings in the `platformio.ini` file.
3. Under the Platform IO menu on the left nav bar, go to esp32cam > Platform > Upload Filesystem Image. 
This will automatically build the react app, modify paths, and gzip it.
5. In esp32dev > General, press Upload or Upload and Monitor. 
