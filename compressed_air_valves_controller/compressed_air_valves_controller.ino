const int NUM_BARRELS = 1;

const int VALVES_INTAKE[] = {2};
const int VALVES_EXHAUST[] = {3};
const int VALVES_WORK[] = {4};

const int SENSORS_LOWER[] = {5};
const int SENSORS_UPPER[] = {6};

enum State { INIT, WORK, EXHAUST, INTAKE } state;


bool water_over_upper_level(int barrel) {
    // pull-up input & sensor upside down, water disconnects it
    return digitalRead(SENSORS_UPPER[barrel]) == HIGH;
}

bool water_over_lower_level(int barrel) {
    // pull-up input, water connects
    return digitalRead(SENSORS_LOWER[barrel]) == LOW;
}

bool not_enough_pressure(int barrel) {
    delay(3000); // TODO make the pressure sensor work
    return 0;
}


void logger(const String &msg) { Serial.println(msg); }

const char* state_to_string(State s) {
    switch (s) {
        case INIT: return "INIT";
        case WORK: return "WORK";
        case EXHAUST: return "EXHAUST";
        case INTAKE: return "INTAKE";
        default: return "UNKNOWN";
    }
}

void log_state() {
    logger(
        "Barrel 0: " + String(state_to_string(state))
        // + " P:" + String("TODO")
        + " L:" + String(digitalRead(SENSORS_LOWER[0]))
        + " U:" + String(digitalRead(SENSORS_UPPER[0]))
    );
}

void step() {
    delay(100);
    log_state();
}


// Relay is low-triggered!
void close_valve(int valve) { digitalWrite(valve, HIGH); }
void open_valve(int valve) { digitalWrite(valve, LOW); }


void init() {
    logger("Init");
    state = INIT;

    open_valve(VALVES_WORK[0]);
    open_valve(VALVES_INTAKE[0]);

    while (water_over_upper_level(0)) step();

    close_valve(VALVES_WORK[0]);

    while (not_enough_pressure(0)) step();

    close_valve(VALVES_INTAKE[0]);

    log_state();
}

void setup() {
    Serial.begin(9600);

    for (int i = 0; i < NUM_BARRELS; i++) {
        pinMode(VALVES_INTAKE[i], OUTPUT);
        pinMode(VALVES_EXHAUST[i], OUTPUT);
        pinMode(VALVES_WORK[i], OUTPUT);

        pinMode(SENSORS_LOWER[i], INPUT_PULLUP);
        pinMode(SENSORS_UPPER[i], INPUT_PULLUP);

        close_valve(VALVES_INTAKE[i]);
        close_valve(VALVES_EXHAUST[i]);
        close_valve(VALVES_WORK[i]);
    }

    init();

    state = WORK;
    logger("Cycle begins");
}


void loop() {
    switch (state) {
        case WORK:
            if (water_over_lower_level(0))
                open_valve(VALVES_WORK[0]);
            else {
                close_valve(VALVES_WORK[0]);
                state = EXHAUST;
            }
            break;

        case EXHAUST:
            if (!water_over_upper_level(0))
                open_valve(VALVES_EXHAUST[0]);
            else {
                close_valve(VALVES_EXHAUST[0]);
                state = INTAKE;
            }
            break;

        case INTAKE:
            if (not_enough_pressure(0))
                open_valve(VALVES_INTAKE[0]);
            else {
                close_valve(VALVES_INTAKE[0]);
                state = WORK;
            }
            break;
    }

    step();
}
