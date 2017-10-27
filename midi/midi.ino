const int PIEZOS = 8;
const int PIEZO_OUT[] = {6, 7, 8, 9, 10, 11, 12, 13};
int piezo_note[PIEZOS];

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


float frequency(int note) {
  return 440 * pow(2, ((note - 69) / 12.0));
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
      float freq = frequency(note);
      debug("Playing " + String(note) + " (" + String(freq) + " Hz) on piezo " + String(PIEZO_OUT[i]));
      tone(PIEZO_OUT[i], freq);
      return;
    }
  }
}


void mute(int note=0) {
  for (int i = 0; i < PIEZOS; i++) {
    if (note == 0 || piezo_note[i] == note) {
      piezo_note[i] = 0;
      debug("Stopping " + String(note) + " on piezo " + String(PIEZO_OUT[i]));
      noTone(PIEZO_OUT[i]);
    }
  }
}

void setup() {
  Serial.begin(9600);
  mute();
}


void serialEvent() {
  int cmd = -1, val;
  while (cmd != 255) {
    cmd = Serial.read();
  }

  cmd = Serial.read();

  if (cmd == 1) {  // play
    val = Serial.read();
    play(val);
  }
  else if (cmd == 2) {  // stop
    val = Serial.read();
    mute(val);
  }

  delay(10);
}


void loop() {
}
