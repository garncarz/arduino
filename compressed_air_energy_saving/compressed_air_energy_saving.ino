#include "constants.h"
#include "logic.h"
#include "commands.h"
#include "wifi.h"

// Arduino-specific analog pin definitions for pressure sensors
const int SENSORS_PRESSURE[MAX_BARRELS] = {A0, A1, A2, A3};

bool pressurized_enough(int barrel_index) {
  return analogRead(SENSORS_PRESSURE[barrel_index]) >= PRESSURE_TARGET;
}
bool water_reached_upper_level(int barrel_index) {
  return digitalRead(SENSORS_UPPER[barrel_index]) == HIGH;  // Upper sensor: HIGH = water present (LOW when no water)
}
bool water_below_lower_level(int barrel_index) {
  return digitalRead(SENSORS_LOWER[barrel_index]) == HIGH;  // Lower sensor: HIGH = no water (water below level)
}

void open_valve(int valve) { digitalWrite(valve, LOW); }  // LOW triggers relay (opens valve)
void close_valve(int valve) { digitalWrite(valve, HIGH); } // HIGH releases relay (closes valve)

#ifndef ARDUINO_UNOR4_WIFI
// Fallback log function for non-WiFi boards
void logger(const String& message) {
  Serial.println(message);
}
#endif

// Check for incoming Serial commands (shared by WiFi and non-WiFi)
void process_serial_commands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    logger("Serial Command received: " + command);
    process_command(command);
  }
}

void setup() {
  Serial.begin(9600);

#ifdef ARDUINO_UNOR4_WIFI
  setup_wifi();
#endif

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

  init_timing_system(); // Initialize timing measurement system

  // Perform startup assessment to recover from potential reset
  assess_startup_state();

  logger("Compressed Air Energy System Started");
}

void print_status() {
  String status = "";
  for (int i = 0; i < NUM_BARRELS; i++) {
    int pressure = analogRead(SENSORS_PRESSURE[i]);
    bool upper_sensor_raw = digitalRead(SENSORS_UPPER[i]);  // Upper: LOW=no water, HIGH=water present
    bool lower_sensor_raw = digitalRead(SENSORS_LOWER[i]);  // Lower: HIGH=no water, LOW=water present

    if (i > 0) status += " | ";
    status += "Barrel" + String(i) + ":" + String(state_name(barrel_states[i]));
    status += " (P:" + String(pressure);
    status += " U:" + String(upper_sensor_raw ? "1" : "0");
    status += " L:" + String(lower_sensor_raw ? "1" : "0") + ")";
  }
  logger(status);
}

void print_timing_stats_arduino() {
  logger("");
  logger("=== TIMING SUMMARY ===");
  for (int i = 0; i < NUM_BARRELS; i++) {
    unsigned long avg_intake = get_average_duration(i, INTAKE);
    unsigned long avg_work = get_average_duration(i, WORK);
    unsigned long avg_exhaust = get_average_duration(i, EXHAUST);
    unsigned long avg_wait_intake = get_average_duration(i, WAIT_FOR_INTAKE);
    unsigned long avg_wait_work = get_average_duration(i, WAIT_FOR_WORK);

    if (avg_intake > 0 || avg_work > 0 || avg_exhaust > 0 || avg_wait_intake > 0 || avg_wait_work > 0) {
      String timing = "Barrel " + String(i) + " averages: ";
      if (avg_intake > 0) timing += "INTAKE=" + String(avg_intake) + "ms ";
      if (avg_work > 0) timing += "WORK=" + String(avg_work) + "ms ";
      if (avg_exhaust > 0) timing += "EXHAUST=" + String(avg_exhaust) + "ms ";
      if (avg_wait_intake > 0) timing += "WAIT_INTAKE=" + String(avg_wait_intake) + "ms ";
      if (avg_wait_work > 0) timing += "WAIT_WORK=" + String(avg_wait_work) + "ms ";
      logger(timing);
    }
  }
  logger("");
}

void print_timing_predictions() {
  logger("=== TIMING PREDICTIONS ===");
  for (int i = 0; i < NUM_BARRELS; i++) {
    unsigned long avg_intake = get_average_duration(i, INTAKE);
    unsigned long avg_work = get_average_duration(i, WORK);
    unsigned long avg_exhaust = get_average_duration(i, EXHAUST);
    unsigned long cycle_time = avg_intake + avg_work + avg_exhaust;

    if (cycle_time > 0) {
      String prediction = "Barrel" + String(i) + " cycle: " + String(cycle_time) +
                         "ms, start INTAKE " + String(avg_work + avg_exhaust) + "ms early";
      logger(prediction);
    }
  }
  logger("");
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
  // Process incoming commands
#ifdef ARDUINO_UNOR4_WIFI
  if (is_wifi_connected()) {
    process_udp_commands();
  }
#endif
  process_serial_commands();

  // Run main logic (respects manual mode)
  logic();

  delay(100);
  print_status();

  // Print timing statistics every 30 seconds
  periodic_timing_report();
}
