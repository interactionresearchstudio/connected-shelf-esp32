#ifndef UTIL_H /* include guards */
#define UTIL_H
#include <Arduino.h>

bool isCredentials();

void initPreferences();

void setCredentials(String SSID, String PASS);

String getPass();

String getSSID();

int getSetting(String name);

void setSetting(String name, int value);

void initButton();

#endif