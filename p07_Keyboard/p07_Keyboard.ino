/*
  Based on: Arduino Starter Kit, Project 7 - Keyboard

  Base parts required:
  - two 10 kilohm resistors
  - 1 megohm resistor
  - 220 ohm resistor
  - four pushbuttons
  - piezo

  Extra parts:
  - 2x potentiometer (1 just for volume adjustment, not used in the code)
*/

const int BASE_TONE = 262;
const int KEY_IN_SCALE[] = {0, 2, 4, 5};

const int KEYBOARD_IN = A0;
const int POTENTIO_IN = A1;
const int PIEZO_OUT = 8;

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


void play(int freq) {
  debug("Playing " + String(freq) + " Hz");
  tone(PIEZO_OUT, freq);
}


void mute() {
  // debug("Muting");
  noTone(PIEZO_OUT);
}


int read_key() {
  int analog = analogRead(KEYBOARD_IN);

  int mapped = 0;
  if (analog == 1023) {
    mapped = 1;
  } else if (analog >= 990 && analog <= 1010) {
    mapped = 2;
  } else if (analog >= 505 && analog <= 515) {
    mapped = 3;
  } else if (analog >= 5 && analog <= 10) {
    mapped = 4;
  }

  // debug("Keyboard read " + String(analog) + ", mapped to " + String(mapped));

  return mapped;
}


float read_potentio() {
  int analog = analogRead(POTENTIO_IN);
  float mapped = (analog / 1023.0 - 0.5) * 2;
  // debug("Potentiometer read " + String(analog) + ", mapped to " + String(mapped));
  return mapped;
}


int get_tone(int key, float pitch_tune) {
  return BASE_TONE * pow(2, (KEY_IN_SCALE[key - 1] + pitch_tune) / 12.0);
}


void setup() {
  Serial.begin(9600);
}


void loop() {
  int key_val = read_key();
  float pitch_tune = read_potentio();

  if (key_val) {
    play(get_tone(key_val, pitch_tune));
  } else {
    mute();
  }
}
