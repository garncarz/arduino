#include "DHT.h"

const int TMP36 = A1;
const int TZ = A2;
const int TR021A = A3;

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


float tmp36ToCelsius(int adcValue) {
  float voltage = adcValue * (5.0 / 1023.0);
  return (voltage - 0.5) * 100.0;
}


float ntcToCelsius(int adcValue) {
  // Constants for your setup
  const float R_FIXED = 10000.0;   // 10 kΩ series resistor
  const float R0      = 14700.0;   // 14.7 kΩ @ 20 °C
  const float T0      = 20.0 + 273.15;  // 20 °C in Kelvin
  const float B       = 3603.3;    // Beta constant

  // 1) Convert ADC to NTC resistance
  float Rntc = R_FIXED * (1023.0 - adcValue) / adcValue;

  // 2) Apply Beta equation
  float T = 1.0 / (1.0 / T0 + (1.0 / B) * log(Rntc / R0));

  // 3) Convert Kelvin to Celsius
  return T - 273.15;
}


float pt100ToCelsius(int adcValue) {
  const float R_FIXED = 100.0;
  float R = R_FIXED * (1023.0 - adcValue) / adcValue;
  return (R - 100.0) / 0.385;
}


void setup() {
  Serial.begin(9600);
  dht.begin();
}


void loop() {
  int val_tmp36 = analogRead(TMP36);
  int val_tr021a = analogRead(TR021A);
  int val_tz = analogRead(TZ);

  float dht_humid = dht.readHumidity();
  float dht_temp = dht.readTemperature();
  float dht_hic = dht.computeHeatIndex(dht_temp, dht_humid, false);
  
  Serial.print("TMP36: " + String(val_tmp36) + " (" + tmp36ToCelsius(val_tmp36) + " °C)");
  Serial.print(" | TR021A: " + String(val_tr021a) + " (" + pt100ToCelsius(val_tr021a) + " °C)");
  Serial.print(" | TZ: " + String(val_tz) + " (" + ntcToCelsius(val_tz) + " °C)");
  Serial.print(" | DHT11 humid/temp/heat index: " + String(dht_humid) + " % / " + String(dht_temp) + " °C / " + String(dht_hic) + " °C");
  Serial.println();
  
  delay(100);
}
