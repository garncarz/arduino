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


float ntcToResistance(int adcValue) {
  return 10000.0 * (1023.0 - adcValue) / adcValue;
}

float ntcRToCelsius(float resistance) {
  const float R0      = 14700.0;   // 14.7 kΩ @ 20 °C
  const float T0      = 20.0 + 273.15;  // 20 °C in Kelvin
  const float B       = 3603.3;    // Beta constant

  // 2) Apply Beta equation
  float T = 1.0 / (1.0 / T0 + (1.0 / B) * log(resistance / R0));

  // 3) Convert Kelvin to Celsius
  return T - 273.15;
}


float pt100ToResistance(int adcValue) {
  return 100.0 * (1023.0 - adcValue) / adcValue;
}

float pt100RToCelsius(float resistance) {
  return (resistance - 100.0) / 0.385;
}


void setup() {
  Serial.begin(9600);
  dht.begin();
}


void loop() {
  int val_tmp36 = analogRead(TMP36);
  float temp_tmp36 = tmp36ToCelsius(val_tmp36);

  int val_tr021a = analogRead(TR021A);
  float res_tr021a = pt100ToResistance(val_tr021a);
  float temp_tr021a = pt100RToCelsius(res_tr021a);

  int val_tz = analogRead(TZ);
  float res_tz = ntcToResistance(val_tz);
  float temp_tz = ntcRToCelsius(res_tz);

  float humid_dht = dht.readHumidity();
  float temp_dht = dht.readTemperature();
  float hic_dht = dht.computeHeatIndex(temp_dht, humid_dht, false);
  
  Serial.print("TMP36: " + String(val_tmp36) + " / " + String(temp_tmp36) + " °C");
  Serial.print(" | TR021A: " + String(val_tr021a) + " / " + String(res_tr021a) + " Ω / " + String(temp_tr021a) + " °C");
  Serial.print(" | TZ: " + String(val_tz) + " / " + String(res_tz) + " Ω / " + String(temp_tz) + "°C");
  Serial.print(" | DHT11: " + String(humid_dht) + " % humid / " + String(temp_dht) + " °C / " + String(hic_dht) + " °C HIC");
  Serial.println();
  
  delay(100);
}
