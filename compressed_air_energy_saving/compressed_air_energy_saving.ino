const int VALVE_INTAKE = 2;
const int VALVE_EXHAUST = 3;
const int VALVE_TO_TURBINE = 4;

const int SENSOR_LOWER = 5;
const int SENSOR_UPPER = 6;

const int SENSOR_PRESSURE = A0;
const int PRESSURE_TARGET = 700;

enum State { INTAKE, WORK, EXHAUST };
State current_state = INTAKE;


bool pressurized_enough() { return analogRead(SENSOR_PRESSURE) >= PRESSURE_TARGET; }
bool water_reached_upper_level() { return digitalRead(SENSOR_UPPER) == LOW; }
bool water_below_lower_level() { return digitalRead(SENSOR_LOWER) == HIGH; }

void open_valve(int valve) { digitalWrite(valve, HIGH); }
void close_valve(int valve) { digitalWrite(valve, LOW); }


void setup() {
  pinMode(VALVE_INTAKE, OUTPUT);
  pinMode(VALVE_EXHAUST, OUTPUT);
  pinMode(VALVE_TO_TURBINE, OUTPUT);

  pinMode(SENSOR_LOWER, INPUT_PULLUP);
  pinMode(SENSOR_UPPER, INPUT_PULLUP);

  close_valve(VALVE_INTAKE);
  close_valve(VALVE_EXHAUST);
  close_valve(VALVE_TO_TURBINE);

  Serial.begin(9600);
}


void loop() {
  switch (current_state) {
    case INTAKE:
      if (pressurized_enough()) {
        close_valve(VALVE_INTAKE);
        current_state = WORK;
      } else open_valve(VALVE_INTAKE);
      break;

    case WORK:
      if (water_below_lower_level()) {
        close_valve(VALVE_TO_TURBINE);
        current_state = EXHAUST;
      } else open_valve(VALVE_TO_TURBINE);
      break;

    case EXHAUST:
      if (water_reached_upper_level()) {
        close_valve(VALVE_EXHAUST);
        current_state = INTAKE;
      } else open_valve(VALVE_EXHAUST);
      break;
  }

  delay(100);
  Serial.println("State: " + String(current_state));
}
