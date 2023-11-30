#ifndef UTIL_H /* include guards */
#define UTIL_H
#include <Arduino.h>

#define DEFAULTNAME "NoName"

bool isCredentials();

void initPreferences();

void setCredentials(String SSID, String PASS);

void setName(String name);

String getName();

String getPass();

String getSSID();

int getSetting(String name);

void setSetting(String name, int value);

void initButton();


#endif