#include "json.h"

String getScanAsJsonString() {
  String jsonString;
  StaticJsonDocument<1000> jsonDoc;
  getScanAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);
  jsonDoc.clear();
  return (jsonString);
}

void getScanAsJson(JsonDocument& jsonDoc) {
  JsonArray networks = jsonDoc.createNestedArray();

  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
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