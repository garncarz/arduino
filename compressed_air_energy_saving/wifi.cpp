#ifdef ARDUINO_UNOR4_WIFI

#include "wifi.h"
#include "commands.h"
#include "WiFiS3.h"
#include "WiFiUdp.h"

// Try to include WiFi credentials, with fallback if file doesn't exist
#if __has_include("wifi_credentials.h")
  #include "wifi_credentials.h"
#else
  const char* WIFI_SSID = "geonika";
  const char* WIFI_PASSWORD = "geo123";
#endif

// UDP target for log messages and commands
const char* UDP_HOST = "255.255.255.255";  // Broadcast
const int UDP_PORT = 1768;  // Single port for both logs and commands

WiFiUDP udp;  // Single UDP instance for both sending and receiving
bool wifi_connected = false;

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
    udp.begin(UDP_PORT);  // Single port for both logs and commands
    Serial.println();
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("UDP port ");
    Serial.print(UDP_PORT);
    Serial.println(" - logs broadcast, commands received");
    Serial.println("Command format: CMD <STATE> <BARREL> or MODE <AUTO/MANUAL>");
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

// Check for incoming UDP commands
void process_udp_commands() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[256];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    buffer[len] = 0;  // Null terminate

    String command = String(buffer);
    log("UDP Command received: " + command);
    process_command(command);
  }
}

bool is_wifi_connected() {
  return wifi_connected;
}

#endif // ARDUINO_UNOR4_WIFI
