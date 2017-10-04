/*
  Base: Arduino Starter Kit, Project 8 - Digital Hourglass
  Code works totally different:
  - Toggles individual LEDs upon receiving their number via Serial.
  - Toggles all LEDs when (the tilt is) shaken.

  Parts required:
  - 10 kilohm resistor
  - six 220 ohm resistors
  - six LEDs
  - tilt switch
*/

const int TILT_IN = 8;
const unsigned long TILT_DELAY = 200;
int tilt_last_value;
unsigned long tilt_last_time;

const int LEDS = 6;
const int LED_OUT[] = {2, 3, 4, 5, 6, 7};
bool led_value[6];

bool DEBUG = 1;


void debug(String msg, bool nl=1, bool force=0) {
  if (not DEBUG and not force) {
    return;
  }

  if (nl) {
    Serial.println(msg);
  } else {
    Serial.print(msg);
  }
}


void set_led(int i, bool value) {
  debug("Setting LED " + String(i) + " to " + String(value));
  led_value[i] = value;

  if (value) {
    digitalWrite(LED_OUT[i], HIGH);
  } else {
    digitalWrite(LED_OUT[i], LOW);
  }
}


void toggle_led(int i) {
  if (i >= 0 && i <= LEDS) {
    set_led(i, !led_value[i]);
  }
}


bool is_tilt_changed() {
  int tilt_value = digitalRead(TILT_IN);
  if (tilt_value != tilt_last_value) {
    tilt_last_value = tilt_value;
    
    if (millis() < tilt_last_time + TILT_DELAY) {
      return 0;
    }
    
    debug("Tilt changed to " + String(tilt_value));
    tilt_last_time = millis();
    return 1;
  }
  
  return 0;
}


void setup() {
  Serial.begin(9600);

  pinMode(TILT_IN, INPUT);
  tilt_last_value = digitalRead(TILT_IN);
  tilt_last_time = millis();
  
  for (int i = 0; i < LEDS; i++) {
    pinMode(LED_OUT[i], OUTPUT);
    set_led(i, 0);
  }
}


void loop() {
  if (is_tilt_changed()) {
    for (int i = 0; i < LEDS; i++) {
      toggle_led(i);
    }
  }
}


void serialEvent() {
  int in = Serial.read();
  toggle_led(in);
}
