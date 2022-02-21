//
// ESPressIoT Controller for Espresso Machines
// (c) 2021 Olaf Kolkman 
// Replaces senspr_max31855 (c) 2016-2021 by Roman Schmitz 

// MAX31855 Sensor Arduino-MAX31855 Library

#ifndef SIMULATION_MODE
#include <SPI.h>
#include "Adafruit_MAX31855.h"


#define ACCURACY 0.3



#define RREF      430.0
#define RNOMINAL  100.0

#define SIM_TIME 100
#define SIM_T_START 20
#define SIM_T_LOSS 2.9e-2
#define SIM_T_HEAT 0.7787



// Example creating a thermocouple instance with software SPI on three digital IO pins
#define MAXDO   3
#define MAXCS   4
#define MAXCLK  5

#define TSIC_SMP_TIME 100

// initialize the Thermocouple
Adafruit_MAX31855 thermoc(MAXCLK, MAXCS, MAXDO);


double lastT = 0.0;
double SumT = 0.0;
double lastErr = 0.0;
int CntT = 0;
unsigned long lastSensTime;

void setupSensor() {

  while (!Serial) delay(1); // wait for Serial on Leonardo/Zero, etc

  Serial.println("MAX31855 test");
  // wait for MAX chip to stabilize
  delay(500);
  Serial.print("Initializing sensor...");
  if (!thermoc.begin()) {
    Serial.println("ERROR.");
    while (1) delay(10);
  }
  Serial.println("DONE.");
}

void updateTempSensor() {

  time_now=millis();

  if ( (max(time_now, lastSensTime) - min(time_now, lastSensTime)) >= TSIC_SMP_TIME) {
  
    double c = thermoc.readCelsius();
    if (isnan(c)) {
       Serial.println("Could not get read temperature, Something wrong with thermocouple!");
    }
    else
    {
      double curT = c;
      // very simple selection of noise hits/invalid values
      if (abs(curT - lastT) < 5.0 || lastT < 1) {
        SumT += curT;
        lastT = curT;
        CntT++;
      }
      lastSensTime = millis();
    }
  }
}

float getTemp() {
  float retVal = gInputTemp;
  if (CntT >= 1) {
    retVal = (SumT / CntT);
    SumT = 0.;
    CntT = 0;
  }
  
  return retVal;
}

#endif
