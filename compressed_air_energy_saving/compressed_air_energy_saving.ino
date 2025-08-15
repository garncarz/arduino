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
  init_timing_system(); // Initialize timing measurement system
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

void print_timing_stats_arduino() {
  Serial.println(); // Add blank line before timing output
  Serial.println("=== TIMING SUMMARY ===");
  for (int i = 0; i < NUM_BARRELS; i++) {
    unsigned long avg_intake = get_average_duration(i, INTAKE);
    unsigned long avg_work = get_average_duration(i, WORK);
    unsigned long avg_exhaust = get_average_duration(i, EXHAUST);
    unsigned long avg_wait_intake = get_average_duration(i, WAIT_FOR_INTAKE);
    unsigned long avg_wait_work = get_average_duration(i, WAIT_FOR_WORK);

    if (avg_intake > 0 || avg_work > 0 || avg_exhaust > 0 || avg_wait_intake > 0 || avg_wait_work > 0) {
      Serial.print("Barrel ");
      Serial.print(i);
      Serial.print(" averages: ");
      if (avg_intake > 0) {
        Serial.print("INTAKE=");
        Serial.print(avg_intake);
        Serial.print("ms ");
      }
      if (avg_work > 0) {
        Serial.print("WORK=");
        Serial.print(avg_work);
        Serial.print("ms ");
      }
      if (avg_exhaust > 0) {
        Serial.print("EXHAUST=");
        Serial.print(avg_exhaust);
        Serial.print("ms ");
      }
      if (avg_wait_intake > 0) {
        Serial.print("WAIT_INTAKE=");
        Serial.print(avg_wait_intake);
        Serial.print("ms ");
      }
      if (avg_wait_work > 0) {
        Serial.print("WAIT_WORK=");
        Serial.print(avg_wait_work);
        Serial.print("ms ");
      }
      Serial.println();
    }
  }
  Serial.println(); // Add blank line after timing output
}

void print_timing_predictions() {
  Serial.println("=== TIMING PREDICTIONS ===");
  for (int i = 0; i < NUM_BARRELS; i++) {
    unsigned long avg_intake = get_average_duration(i, INTAKE);
    unsigned long avg_work = get_average_duration(i, WORK);
    unsigned long avg_exhaust = get_average_duration(i, EXHAUST);
    unsigned long cycle_time = avg_intake + avg_work + avg_exhaust;

    if (cycle_time > 0) {
      Serial.print("Barrel");
      Serial.print(i);
      Serial.print(" cycle: ");
      Serial.print(cycle_time);
      Serial.print("ms, start INTAKE ");
      Serial.print(avg_work + avg_exhaust);
      Serial.println("ms early");
    }
  }
  Serial.println(); // Add blank line after predictions
}// Call this function periodically (e.g., every 30 seconds) to print timing data
unsigned long last_timing_print = 0;
void periodic_timing_report() {
  if (millis() - last_timing_print > 30000) { // Every 30 seconds
    print_timing_stats_arduino();
    print_timing_predictions();
    last_timing_print = millis();
  }
}


void loop() {
  logic();

  delay(100);
  print_status();

  // Print timing statistics every 30 seconds
  periodic_timing_report();
}
