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


float frequency(int tone_number) {
  return 440 * pow(2, ((tone_number - 69) / 12.0));
}


void play(int freq) {
  debug("Playing " + String(freq) + " Hz");
  tone(PIEZO_OUT, freq);
}


void mute() {
  // debug("Muting");
  noTone(PIEZO_OUT);
}


void setup() {
  Serial.begin(9600);
}


void serialEvent() {
  int cmd = 255, val;
  while (cmd == 255) {
    cmd = Serial.read();
  }

  if (cmd == 1) {  // play
    val = Serial.read();
    play(frequency(val));
  }
  else if (cmd == 2) {  // stop
    mute();
  }

  delay(10);
}


void loop() {
}
