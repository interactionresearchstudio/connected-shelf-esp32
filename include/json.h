#ifndef JSON_H
#define JSON_H

#include "Arduino.h"
#include "util.h"
#include "ArduinoJson.h"
#include "WiFi.h"
#include "main.h"
#include "pins.h"


String loadJSON();
void updateJson(const char* jsonIn);
String getScanAsJsonString();
void getScanAsJson(JsonDocument& jsonDoc);
void getStatusAsJson(JsonDocument& jsonDoc);
String getStatusAsJsonString();
#endif