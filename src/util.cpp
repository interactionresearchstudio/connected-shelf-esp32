#include "util.h"
#include <pins.h>
#include <Arduino.h> /* we need arduino functions to implement this */
#include <Preferences.h>
Preferences prefs;

bool isCredentials(){
    if(prefs.getString("SSID", "") == ""){
        return false;
    } else {
        return true;
    }
}

void initPreferences(){
   prefs.begin("connected-shelf", false);
}

void setCredentials(String SSID, String PASS){
    prefs.putString("SSID", SSID);
    prefs.putString("PASS", PASS);
}

String getPass(){
    return prefs.getString("PASS");
}

String getSSID(){
    return prefs.getString("SSID");
}

int getSetting(String name){
    return prefs.getInt(name.c_str(), 99999);
}

void setSetting(String name, int value){
    prefs.putInt(name.c_str(), value);
}

void initButton(){
    pinMode(USER_BUTTON,INPUT_PULLUP);
}