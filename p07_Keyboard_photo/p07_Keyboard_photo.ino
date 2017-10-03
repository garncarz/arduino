/*
  Based on: Arduino Starter Kit, Project 7 - Keyboard
  Adds a pitch tuning photoresistor.

  Base parts required:
  - two 10 kilohm resistors
  - 1 megohm resistor
  - 220 ohm resistor
  - four pushbuttons
  - piezo

  Extra parts:
  - potentiometer (just for volume adjustment, not used in the code)
  - photoresistor
  - 10 kilohm resistor
*/

const int BASE_TONE = 262;
const int KEY_IN_SCALE[] = {0, 3, 5, 7};

const int INFO_OUT = 13;
const int KEYBOARD_IN = A0;
const int PHOTO_IN = A1;
const int PIEZO_OUT = 8;

bool DEBUG = 1;

int photo_min, photo_max;


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


void calibrate_photo(unsigned long end_time) {
  debug("Calibrating photoresistor...");
  
  photo_min = 1023;
  photo_max = 0;

  while (millis() < end_time) {
    int value = analogRead(PHOTO_IN);
    if (value > photo_max) {
      photo_max = value;
    }
    if (value < photo_min) {
      photo_min = value;
    }
    delay(10);
  }

  debug("Photo min: " + String(photo_min) + ", photo max: " + String(photo_max));
}


float read_photo() {
  int analog = analogRead(PHOTO_IN);
  float mapped = ((float)analog - photo_min) / (photo_max - photo_min);
  debug("Photoresistor read " + String(analog) + ", mapped to " + String(mapped));
  return mapped;
}


int get_tone(int key, float pitch_tune) {
  return BASE_TONE * pow(2, (KEY_IN_SCALE[key - 1] + 1 - pitch_tune) / 12.0);
}


void setup() {
  Serial.begin(9600);

  pinMode(INFO_OUT, OUTPUT);
  digitalWrite(INFO_OUT, HIGH);

  calibrate_photo(millis() + 5000);

  digitalWrite(INFO_OUT, LOW);
}


void loop() {
  int key_val = read_key();
  float pitch_tune = read_photo();

  if (key_val) {
    play(get_tone(key_val, pitch_tune));
  } else {
    mute();
  }

  delay(10);
}
