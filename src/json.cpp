#include "json.h"

String getScanAsJsonString() {
  String jsonString;
  StaticJsonDocument<1000> jsonDoc;
  getScanAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);
  jsonDoc.clear();
  return (jsonString);
}

String getStatusAsJsonString() {

  String jsonString;
  StaticJsonDocument<200> jsonDoc;
  getStatusAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);
  jsonDoc.clear();
  return (jsonString);
}

String getWifiString() {
  String out;
  if (getInternetStatus() == true) {
    out = "[{\"connected\": true}]";
  } else {
    out = "[{\"connected\": false}]";
  }
  return out;
}

void getScanAsJson(JsonDocument& jsonDoc) {
  JsonArray networks = jsonDoc.createNestedArray();

  WiFi.disconnect();
  //WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks();
  if(n > 5) n = 5;

  //Array is ordered by signal strength - strongest first
  for (int i = 0; i < n; ++i) {
    String networkSSID = WiFi.SSID(i);
    if (networkSSID.length() <= SSID_MAX_LENGTH) {
      JsonObject network  = networks.createNestedObject();
      network["SSID"] = WiFi.SSID(i);
      //network["BSSID"] = WiFi.BSSIDstr(i);
      //network["RSSI"] = WiFi.RSSI(i);
    }
  }
}

void getStatusAsJson(JsonDocument& jsonDoc) {
  JsonArray statuses = jsonDoc.createNestedArray();
      JsonObject status  = statuses.createNestedObject();
      status["connected"] = getInternetStatus();
      status["displayName"] = getName();
      //network["BSSID"] = WiFi.BSSIDstr(i);
      //network["RSSI"] = WiFi.RSSI(i);
}
