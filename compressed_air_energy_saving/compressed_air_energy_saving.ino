#include "constants.h"
#include "logic.h"

// Arduino-specific analog pin definitions for pressure sensors
const int SENSORS_PRESSURE[MAX_BARRELS] = {A0, A1, A2, A3};

bool pressurized_enough(int barrel_index) {
  return analogRead(SENSORS_PRESSURE[barrel_index]) >= PRESSURE_TARGET;
}
bool water_reached_upper_level(int barrel_index) {
  return digitalRead(SENSORS_UPPER[barrel_index]) == LOW;
}
bool water_below_lower_level(int barrel_index) {
  return digitalRead(SENSORS_LOWER[barrel_index]) == HIGH;
}

void open_valve(int valve) { digitalWrite(valve, HIGH); }
void close_valve(int valve) { digitalWrite(valve, LOW); }


void setup() {
  // Setup all barrels
  for (int i = 0; i < NUM_BARRELS; i++) {
    pinMode(VALVES_INTAKE[i], OUTPUT);
    pinMode(VALVES_EXHAUST[i], OUTPUT);
    pinMode(VALVES_TO_TURBINE[i], OUTPUT);
    pinMode(SENSORS_LOWER[i], INPUT_PULLUP);
    pinMode(SENSORS_UPPER[i], INPUT_PULLUP);

    // Initialize all valves closed
    close_valve(VALVES_INTAKE[i]);
    close_valve(VALVES_EXHAUST[i]);
    close_valve(VALVES_TO_TURBINE[i]);
  }

  Serial.begin(9600);
}

void print_status() {
  for (int i = 0; i < NUM_BARRELS; i++) {
    int pressure = analogRead(SENSORS_PRESSURE[i]);
    bool upper_triggered = (digitalRead(SENSORS_UPPER[i]) == LOW);
    bool lower_triggered = (digitalRead(SENSORS_LOWER[i]) == LOW);

    if (i > 0) Serial.print(" | ");
    Serial.print("Barrel");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(state_name(barrel_states[i]));
    Serial.print(" (P:");
    Serial.print(pressure);
    Serial.print(" U:");
    Serial.print(upper_triggered ? "1" : "0");
    Serial.print(" L:");
    Serial.print(lower_triggered ? "1" : "0");
    Serial.print(")");
  }
  Serial.println();
}


void loop() {
  logic();

  delay(100);
  print_status();
}
