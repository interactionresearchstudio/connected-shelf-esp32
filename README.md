# ESP32 React Web Server with websockets

This template repository consists of a React app with a single button that when pressed will 
blink an ESP32 dev board's builtin LED. It's based off [h9419's repo](https://github.com/h9419/ESP_AP_Webserver), 
but with a few improvements. 

## Networking

The ESP32 creates an access point and runs a captive portal at the IP `192.168.2.1`. A websocket connection 
is maintained between the React app and the ESP32 to pass events back and forth as JSON objects. 

## How to upload

1. Install Platform IO.
2. If needed, modify upload and port settings in the `platformio.ini` file.
3. Under the Platform IO menu on the left nav bar, go to esp32dev > Platform > Upload Filesystem Image. 
This will automatically build the react app, modify paths, and gzip it.
4. In esp32dev > General, press Upload or Upload and Monitor. 
5. Connect to the ESP32 access point. A captive portal should show up with a button that blinks the onboard LED!