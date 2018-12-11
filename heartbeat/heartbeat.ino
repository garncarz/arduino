const int IR_IN = A0;
const int LED_OUT = 2;
const int PIEZO_OUT = 3;

bool DEBUG = 1;

int ir_unused_min, ir_unused_max;

const int IR_VALUES_COUNT = 800;
int ir_values[IR_VALUES_COUNT];


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


void calibrate_ir(unsigned long end_time) {
  debug("Calibrating infrared sensor...");

  ir_unused_min = 1023;
  ir_unused_max = 0;

  while (millis() < end_time) {
    int value = analogRead(IR_IN);
    if (value > ir_unused_max) {
      ir_unused_max = value;
    }
    if (value < ir_unused_min) {
      ir_unused_min = value;
    }
    delay(10);
  }

  debug("IR unused min: " + String(ir_unused_min) + ", IR unused max: " + String(ir_unused_max));
}


bool was_ir_peak() {
    const int IR_IMPORTANT_VALUE_INDEX = IR_VALUES_COUNT - 20;

    if (ir_values[IR_IMPORTANT_VALUE_INDEX] <= ir_unused_max) {
        return false;
    }

    for (int i = 0; i < IR_VALUES_COUNT; i++) {
        if (i == IR_IMPORTANT_VALUE_INDEX) {
            continue;
        }
        if (ir_values[i] > ir_values[IR_IMPORTANT_VALUE_INDEX]) {
            return false;
        }
    }

    return true;
}


bool is_beat_on() {
    if (ir_values[IR_VALUES_COUNT - 1] <= ir_unused_max) {
        return false;
    }

    int avg, sum = 0;
    int min_val = 1023, max_val = 0;

    for (int i = 0; i < IR_VALUES_COUNT; i++) {
        sum += ir_values[i];
        if (ir_values[i] > max_val) {
            max_val = ir_values[i];
        }
        if (ir_values[i] < min_val) {
            min_val = ir_values[i];
        }
    }

    avg = sum / IR_VALUES_COUNT;

    if (ir_values[IR_VALUES_COUNT - 1] > avg * (max_val - min_val - avg) * 0.3) {
        return true;
    }

    return false;
}


bool process_ir() {
    int ir_value = analogRead(IR_IN);
    // debug(String(ir_value));

    // is it a new value?
    if (ir_value != ir_values[IR_VALUES_COUNT - 1]) {
        // let's shift old values and append the new one
        for (int i = 0; i < IR_VALUES_COUNT - 1; i++) {
            ir_values[i] = ir_values[i + 1];
        }
        ir_values[IR_VALUES_COUNT - 1] = ir_value;

        return is_beat_on();
    }

    return false;
}


void setup() {
    Serial.begin(9600);

    for (int i = 0; i < IR_VALUES_COUNT; i++) {
        ir_values[i] = 0;
    }

    pinMode(LED_OUT, OUTPUT);
    digitalWrite(LED_OUT, LOW);

    calibrate_ir(millis() + 5000);
}


void loop() {
    bool beat_on = process_ir();

    debug(beat_on ? "x" : ".", false);
    digitalWrite(LED_OUT, beat_on ? HIGH : LOW);

    if (beat_on) {
        tone(PIEZO_OUT, 80);
    } else {
        noTone(PIEZO_OUT);
    }

    delay(10);
}
