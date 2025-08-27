#include "constants.h"
#include "logic.h"

// WiFi support for Arduino Uno R4 WiFi
#ifdef ARDUINO_UNOR4_WIFI
#include "WiFiS3.h"
#include "WiFiUdp.h"

// Try to include WiFi credentials, with fallback if file doesn't exist
#if __has_include("wifi_credentials.h")
  #include "wifi_credentials.h"
#else
  const char* WIFI_SSID = "geonika";
  const char* WIFI_PASSWORD = "geo123";
#endif

// UDP target for log messages
const char* UDP_HOST = "255.255.255.255";  // Broadcast
const int UDP_PORT = 1768;

WiFiUDP udp;
bool wifi_connected = false;
#endif

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

void open_valve(int valve) { digitalWrite(valve, LOW); }  // LOW triggers relay (opens valve)
void close_valve(int valve) { digitalWrite(valve, HIGH); } // HIGH releases relay (closes valve)

#ifdef ARDUINO_UNOR4_WIFI
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  for (int attempts = 0; attempts < 20 && WiFi.status() != WL_CONNECTED; attempts++) {
    delay(500);
    Serial.print(".");
    Serial.flush();  // Ensure dots appear immediately
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    udp.begin(UDP_PORT);
    Serial.println();
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Log messages will be sent to: ");
    Serial.print(UDP_HOST);
    Serial.print(":");
    Serial.println(UDP_PORT);
  } else {
    Serial.println();
    Serial.println("WiFi connection failed - continuing with Serial only");
  }
}

// Send log message via both Serial and WiFi UDP
void log(const String& message) {
  // Always print to Serial
  Serial.println(message);

  // Also send via WiFi if connected
  if (wifi_connected && WiFi.status() == WL_CONNECTED) {
    udp.beginPacket(UDP_HOST, UDP_PORT);
    udp.println(message);
    udp.endPacket();
  }
}
#else
// Fallback for non-WiFi boards
void log(const String& message) {
  Serial.println(message);
}
#endif


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

  log("Compressed Air Energy System Started");
}

void print_status() {
  String status = "";
  for (int i = 0; i < NUM_BARRELS; i++) {
    int pressure = analogRead(SENSORS_PRESSURE[i]);
    bool upper_triggered = (digitalRead(SENSORS_UPPER[i]) == LOW);
    bool lower_triggered = (digitalRead(SENSORS_LOWER[i]) == LOW);

    if (i > 0) status += " | ";
    status += "Barrel" + String(i) + ":" + String(state_name(barrel_states[i]));
    status += " (P:" + String(pressure);
    status += " U:" + String(upper_triggered ? "1" : "0");
    status += " L:" + String(lower_triggered ? "1" : "0") + ")";
  }
  log(status);
}

void print_timing_stats_arduino() {
  log("");
  log("=== TIMING SUMMARY ===");
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
      log(timing);
    }
  }
  log("");
}

void print_timing_predictions() {
  log("=== TIMING PREDICTIONS ===");
  for (int i = 0; i < NUM_BARRELS; i++) {
    unsigned long avg_intake = get_average_duration(i, INTAKE);
    unsigned long avg_work = get_average_duration(i, WORK);
    unsigned long avg_exhaust = get_average_duration(i, EXHAUST);
    unsigned long cycle_time = avg_intake + avg_work + avg_exhaust;

    if (cycle_time > 0) {
      String prediction = "Barrel" + String(i) + " cycle: " + String(cycle_time) +
                         "ms, start INTAKE " + String(avg_work + avg_exhaust) + "ms early";
      log(prediction);
    }
  }
  log("");
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
