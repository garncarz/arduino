#include "TimerOne.h"

const int PIEZOS = 8;
const int PIEZO_OUT[] = {6, 7, 8, 9, 10, 11, 12, 13};
int piezo_note[PIEZOS], piezo_val[PIEZOS];
double piezo_freq[PIEZOS];
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


double frequency(int note) {
  return 440 * pow(2, ((note - 69) / 12.0));
}


int sign_digital(double val) {
  if (val > 0) {
    return HIGH;
  }
  return LOW;
}


void play(int note) {
  for (int i = 0; i < PIEZOS; i++) {
    if (piezo_note[i] == note) {
      return;
    }
  }

  for (int i = 0; i < PIEZOS; i++) {
    if (piezo_note[i] == 0) {
      piezo_note[i] = note;
      piezo_freq[i] = frequency(note);
      debug("Playing " + String(note) + " (" + String(piezo_freq[i]) + " Hz) on piezo " + String(PIEZO_OUT[i]));
      return;
    }
  }
}


void mute(int note=0) {
  for (int i = 0; i < PIEZOS; i++) {
    if (note == 0 || piezo_note[i] == note) {
      piezo_note[i] = 0;
      debug("Stopping " + String(note) + " on piezo " + String(PIEZO_OUT[i]));
    }
  }
}

void setup() {
  Serial.begin(9600);
  
  for (int i = 0; i < PIEZOS; i++) {
    pinMode(PIEZO_OUT[i], OUTPUT);
    digitalWrite(PIEZO_OUT[i], LOW);
    piezo_note[i] = 0;
    piezo_val[i] = LOW;
  }
  
  Timer1.initialize(200);
  Timer1.attachInterrupt(tick);
}


void loop() {
}


void serialEvent() {
  if (Serial.available() >= 3) {
    int cmd = -1, val;
    
    do {
      cmd = Serial.read();
    } while (cmd != 255);
  
    cmd = Serial.read();
  
    if (cmd == 1) {  // play
      val = Serial.read();
      play(val);
    }
    else if (cmd == 2) {  // stop
      val = Serial.read();
      mute(val);
    }
  }
}


void safe_piezo_write(int i, int val) {
  if (piezo_val[i] != val) {
    digitalWrite(PIEZO_OUT[i], val);
    piezo_val[i] = val;
  }
}


void tick() {
  int piezo_nr = millis() % PIEZOS;
  if (piezo_note[piezo_nr]) {
    safe_piezo_write(piezo_nr, sign_digital(sin(micros() / 1000000.0 * piezo_freq[piezo_nr])));
  } else {
    safe_piezo_write(piezo_nr, LOW);
  }
}
