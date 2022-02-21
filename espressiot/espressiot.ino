#include <WiFi.h>


#include <ArduinoJson.h>

//
// ESPressIoT Controller for Espresso Machines
// 2016-2021 by Roman Schmitz
//
// Uses PID library
//

#include <PID_v1.h>


// WIFI
//
// please set your wifi SSID and password in WiFiSecrets.h
#include "WiFiSecrets.h"

#define MAX_CONNECTION_RETRIES 20

// options for special modules
#define ENABLE_JSON
#define ENABLE_HTTP
#define ENABLE_TELNET
#define ENABLE_MQTT
#define ENABLE_SERIAL

// enable detectio of an external switch/thermostat
//#define ENABLE_SWITCH_DETECTION

// use simulation or real heater and sensors
//#define SIMULATION_MODE

//
// STANDARD reset values based on Gaggia CC
//
#define S_P 287.11
#define S_I 579.10
#define S_D 800.00
#define S_aP 100.0
#define S_aI 0.0
#define S_aD 0.0
#define S_TSET 95.5
#define S_TBAND 1.5

//
// Intervals for I/O
//
#define HEATER_INTERVAL 1000
#define DISPLAY_INTERVAL 1000
#define PID_INTERVAL 100

//
// global variables
//
double gTargetTemp = S_TSET;
double gOvershoot = S_TBAND;
double gInputTemp = 20.0;
double gOutputPwr = 0.0;
double gP = S_P, gI = S_I, gD = S_D;
double gaP = S_aP, gaI = S_aI, gaD = S_aD;

unsigned long time_now = 0;
unsigned long time_last = 0;

int gButtonState = 0;
uint8_t mac[6];

boolean tuning = false;
boolean osmode = false;
boolean poweroffMode = false;
boolean externalControlMode = false;

String gStatusAsJson;

//
// gloabl classes
//
PID ESPPID(&gInputTemp, &gOutputPwr, &gTargetTemp, gP, gI, gD, DIRECT);

void setup()
{
  gOutputPwr = 0;

  Serial.begin(115200);
  // Need to print something to get the serial monitor setled
  for (int i = 0; i < 5 ; i++){
    Serial.print("Initializing (");
    Serial.print(i);
    Serial.println(")");
    delay(100);
  }
  
  
  
  Serial.println("Mounting SPIFFS...");
  if (!prepareFS()) {
    Serial.println("Failed to mount SPIFFS !");
  } else {
    Serial.println("Mounted.");
  }

  Serial.println("Loading config...");
  if (!loadConfig()) {
    Serial.println("Failed to load config. Using default values and creating config...");
    if (!saveConfig()) {
      Serial.println("Failed to save config");
    } else {
      Serial.println("Config saved");
    }
  } else {
    Serial.println("Config loaded");
  }

  Serial.println("Settin up PID...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.macAddress(mac);
  Serial.println("");
  Serial.print("MAC address: ");
  Serial.println(macToString(mac));

  Serial.print("Connecting to Wifi AP");
  for (int i = 0; i < MAX_CONNECTION_RETRIES && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Error connection to AP after ");
    Serial.print(MAX_CONNECTION_RETRIES);
    Serial.println(" retries.");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
   delay(2);
    Serial.println("something");

  

#ifdef ENABLE_TELNET
  setupTelnet();
#endif

#ifdef ENABLE_HTTP
  setupWebSrv();
#endif

#ifdef ENABLE_MQTT
  setupMQTT();
#endif

#ifdef ENABLE_SWITCH_DETECTION
  setupSwitch();
#endif

  // setup components
  setupHeater();
  setupSensor();

  // start PID
  ESPPID.SetTunings(gP, gI, gD);
  ESPPID.SetSampleTime(PID_INTERVAL);
  ESPPID.SetOutputLimits(0, 1000);
  ESPPID.SetMode(AUTOMATIC);

  time_now = millis();
  time_last = time_now;

}

void serialStatus() {
  Serial.println(gStatusAsJson);
}

void loop() {
  time_now = millis();
  updateTempSensor();
  gInputTemp = getTemp();
  Serial.println(gInputTemp);

#ifdef ENABLE_SWITCH_DETECTION
  loopSwitch();
#endif

  if ( (max(time_now, time_last) - min(time_now, time_last)) >= PID_INTERVAL or time_last > time_now) {
    if (poweroffMode == true) {
      gOutputPwr = 0;
      setHeatPowerPercentage(gOutputPwr);
    }
    else if (externalControlMode == true) {
      gOutputPwr = 1000 * gButtonState;
      setHeatPowerPercentage(gOutputPwr);
    }
    else if (tuning == true)
    {
      tuning_loop();
    }
    else  {
      if ( !osmode && abs(gTargetTemp - gInputTemp) >= gOvershoot ) {
        ESPPID.SetTunings(gaP, gaI, gaD);
        osmode = true;
      }
      else if ( osmode && abs(gTargetTemp - gInputTemp) < gOvershoot ) {
        ESPPID.SetTunings(gP, gI, gD);
        osmode = false;
      }
      if (ESPPID.Compute() == true) {
        setHeatPowerPercentage(gOutputPwr);
      }
    }

    // create status String (JSON)
    gStatusAsJson = statusAsJson();

#ifdef ENABLE_MQTT
    loopMQTT();
#endif

#ifdef ENABLE_TELNET
    loopTelnet();
#endif

#ifdef ENABLE_SERIAL
    serialStatus();
#endif

    time_last = time_now;
  }

  updateHeater();

#ifdef ENABLE_HTTP
  loopWebSrv();
#endif

}
