const int INFO_OUT = 13;
const int PHOTOS = 3;
const int PHOTO_IN[PHOTOS] = {A0, A1, A2};
const int BUTTONS = 3;
const int BUTTON_IN[BUTTONS] = {2, 3, 4};

bool DEBUG = 0;

int photo_min[PHOTOS], photo_max[PHOTOS];
int button_value[BUTTONS];


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


void calibrate_photos(unsigned long end_time) {
  debug("Calibrating photoresistors...");

  for (int i = 0; i < PHOTOS; i++) {
    photo_min[i] = 1023;
    photo_max[i] = 0;
  }

  while (millis() < end_time) {
    for (int i = 0; i < PHOTOS; i++) {
      int value = analogRead(PHOTO_IN[i]);
      if (value > photo_max[i]) {
        photo_max[i] = value;
      }
      if (value < photo_min[i]) {
        photo_min[i] = value;
      }
    }
  }

  for (int i = 0; i < PHOTOS; i++) {
    debug("Photo #" + String(i) + ": min=" + String(photo_min[i]) + " max=" + String(photo_max[i]));
  }
}


float read_photo(int i) {
  int analog = analogRead(PHOTO_IN[i]);
  float mapped = ((float)analog - photo_min[i]) / (photo_max[i] - photo_min[i]);
  debug("Photoresistor #" + String(i) + " read " + String(analog) + ", mapped to " + String(mapped));
  return mapped;
}


int read_button(int i) {
  int val = digitalRead(BUTTON_IN[i]);
  int ret = 0;
  if (val && !button_value[i]) {
    ret = 1;
  }
  else if (!val && button_value[i]) {
    ret = 2;
  }
  button_value[i] = val;
  return ret;
}


int map_photo(int photo_nr, float value) {
  int dir = 0;
  
  if (value < 0.65) {
    dir = 1;
  }
  else if (value > 0.70 && value < 0.95) {
    dir = 2;
  }

  if (!dir) {
    return 0;
  }

  return photo_nr * 10 + dir;
}


int map_button(int button_nr, int value) {
  if (!value) {
    return 0;
  }
  
  return (button_nr + PHOTOS) * 10 + value;
}


void send_val(int value) {
  if (value) {
    debug("Sending " + String(value));
    if (!DEBUG) {
      Serial.write(value);
    }
  }
}


void setup() {
  Serial.begin(9600);

  pinMode(INFO_OUT, OUTPUT);
  digitalWrite(INFO_OUT, HIGH);

  for (int i = 0; i < BUTTONS; i++) {
    pinMode(BUTTON_IN[i], INPUT);
    button_value[i] = 0;
  }

  calibrate_photos(millis() + 5000);

  digitalWrite(INFO_OUT, LOW);
}


void loop() {
  for (int i = 0; i < PHOTOS; i++) {
    send_val(map_photo(i, read_photo(i)));
  }

  for (int i = 0; i < BUTTONS; i++) {
    send_val(map_button(i, read_button(i)));
  }

  if (DEBUG) {
    delay(1000);
  }
}
