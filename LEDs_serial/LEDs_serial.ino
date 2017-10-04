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
bool led_value[LEDS], old_led_value[LEDS];

const int DELAY = 100;

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


int mod(int a, int b=LEDS) {
  return ((a % b) + b) % b;
}


void set_led(int i, bool value) {
  i = mod(i);
  
  debug("Setting LED " + String(i) + " to " + String(value));
  led_value[i] = value;

  if (value) {
    digitalWrite(LED_OUT[i], HIGH);
  } else {
    digitalWrite(LED_OUT[i], LOW);
  }
}


bool get_led(int i) {
  return led_value[mod(i)];
}


void toggle_led(int i) {
  set_led(i, !get_led(i));
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
  switch(in) {
    case 20: shift_up(); break;  // left, up
    case 21: shift_down(); break;  // right, down
    case 22: blink_off(); break;  // space
    case 23: blink_off_on(); break;  // enter
    case 24: backspace(); break;  // backspace
    case 25: del(); break;  // delete
    case 26: pg_up(); break;  // page up
    case 27: pg_down(); break;  // page down
    case 28: blink_one(0); break;  // home
    case 29: blink_one(-1); break;  // end
    default: toggle_led(in);
  }
}


bool get_old_led(int i) {
  return old_led_value[mod(i)];
}


void backup_leds() {
  for (int i = 0; i < LEDS; i++) {
    old_led_value[i] = led_value[i];
  }
}


void restore_leds() {
  for (int i = 0; i < LEDS; i++) {
    set_led(i, get_old_led(i));
  }
}


void shift_up() {
  backup_leds();
  for (int i = 1; i <= LEDS; i++) {
    set_led(i, get_old_led(i - 1));
  }
}

void shift_down() {
  backup_leds();
  for (int i = LEDS - 2; i >= -1; i--) {
    set_led(i, get_old_led(i + 1));
  }
}


void blink_off() {
  backup_leds();
  
  for (int i = 0; i < LEDS; i++) {
    set_led(i, 0);
  }
  delay(DELAY);
  
  for (int i = 0; i < LEDS; i++) {
    set_led(i, get_old_led(i));
  }
}


void blink_off_on() {
  backup_leds();
  
  for (int i = 0; i < LEDS; i++) {
    set_led(i, 0);
  }
  delay(DELAY / 2);
  
  for (int i = 0; i < LEDS; i++) {
    set_led(i, 1);
  }
  delay(DELAY / 2);
  
  for (int i = 0; i < LEDS; i++) {
    set_led(i, get_old_led(i));
  }
}


void backspace() {
  for (int i = LEDS - 1; i >= 0; i--) {
    if (get_led(i)) {
      set_led(i, 0);
      break;
    }
  }
}


void del() {
  for (int i = 0; i < LEDS - 1; i++) {
    set_led(i, get_led(i + 1));
  }
  set_led(LEDS - 1, 0);
}


void pg_up() {
  for (int i = 0; i < LEDS; i++) {
    shift_up();
    delay(DELAY / LEDS);
  }
}


void pg_down() {
  for (int i = 0; i < LEDS; i++) {
    shift_down();
    delay(DELAY / LEDS);
  }
}


void blink_one(int i) {
  int old = get_led(i);

  set_led(i, 1);
  delay(DELAY / 5);
  set_led(i, 0);
  delay(DELAY / 5);
  set_led(i, 1);
  delay(DELAY / 5);
  set_led(i, 0);
  delay(DELAY / 5);
  set_led(i, 1);
  delay(DELAY / 5);
  
  set_led(i, old);
}
