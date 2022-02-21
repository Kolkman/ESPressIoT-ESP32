//
// ESPressIoT Controller for Espresso Machines
// 2016-2021 by Roman Schmitz
//
// JSON Config File on SPIFFS
//

#ifdef ENABLE_JSON

#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <SPIFFS.h>

#define BUF_SIZE 1024

bool prepareFS() {
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return false;
  }
  return true;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  DynamicJsonDocument jsonDocument(BUF_SIZE);
  ReadLoggingStream loggingStream(configFile, Serial);

  DeserializationError parsingError = deserializeJson(jsonDocument, loggingStream);
  if (parsingError) {
    Serial.println("Failed to deserialize json config file");
    Serial.println(parsingError.c_str());
    return false;
  }

  //wifi_ssid = jsonDocument["ssid"];
  //wifi_pass = jsonDocument["password"];
  gTargetTemp = jsonDocument["tset"];
  gOvershoot = jsonDocument["tband"];
  gP = jsonDocument["P"], gI = jsonDocument["I"], gD = jsonDocument["D"];
  gaP = jsonDocument["aP"], gaI = jsonDocument["aI"], gaD = jsonDocument["aD"];

  return true;
}

bool saveConfig() {
  DynamicJsonDocument jsonDocument(BUF_SIZE);

  //jsonDocument["ssid"] = wifi_ssid;  jsonDocument["password"] = wifi_pass;
  jsonDocument["tset"] = gTargetTemp;  jsonDocument["tband"] = gOvershoot;
  jsonDocument["P"] = gP, jsonDocument["I"] = gI, jsonDocument["D"] = gD;
  jsonDocument["aP"] = gaP, jsonDocument["aI"] = gaI, jsonDocument["aD"] = gaD;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  WriteLoggingStream loggedFile(configFile, Serial);

  size_t writtenBytes = serializeJson(jsonDocument, loggedFile);

  if (writtenBytes == 0) {
    Serial.println(F("Failed to write to file"));
    return false;
  }
  Serial.println("Bytes written: " + String(writtenBytes));

  configFile.close();
  return true;
}

void resetConfig() {
  gP = S_P; gI = S_I; gD = S_D;
  gaP = S_aP; gaI = S_aI; gaD = S_aD;
  gTargetTemp = S_TSET;
  gOvershoot = S_TBAND;
}

#endif
