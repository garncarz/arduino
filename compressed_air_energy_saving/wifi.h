#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>

#ifdef ARDUINO_UNOR4_WIFI
void setup_wifi();
void process_udp_commands();
bool is_wifi_connected();
void logger(const String& message);  // WiFi version of log
#endif

#endif // WIFI_H
