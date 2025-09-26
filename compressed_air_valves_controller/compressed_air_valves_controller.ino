const int NUM_BARRELS = 2;

const int VALVES_INTAKE[] = {2, 7};
const int VALVES_EXHAUST[] = {3, 8};
const int VALVES_WORK[] = {4, 9};

const int SENSORS_LOWER[] = {5, 10};
const int SENSORS_UPPER[] = {6, 11};

enum State { INIT, WORK, EXHAUST, INTAKE, READY_FOR_WORK } state[NUM_BARRELS];


bool water_over_upper_level(int barrel) {
    // sensor upside down, water disconnects it
    return digitalRead(SENSORS_UPPER[barrel]) == LOW;
}

bool water_over_lower_level(int barrel) {
    // water connects
    return digitalRead(SENSORS_LOWER[barrel]) == HIGH;
}

bool not_enough_pressure(int barrel) {
    delay(1000); // TODO make the pressure sensor work
    return 0;
}


void logger(const String &msg) { Serial.println(msg); }

const char* state_to_string(State s) {
    switch (s) {
        case INIT: return "INIT";
        case WORK: return "WORK";
        case EXHAUST: return "EXHAUST";
        case INTAKE: return "INTAKE";
        case READY_FOR_WORK: return "READY";
        default: return "UNKNOWN";
    }
}

void log_state() {
    char line[128] = "";
    char buf[64];

    for (int i = 0; i < NUM_BARRELS; i++) {
        snprintf(
            buf, sizeof(buf),
            "Barrel %d: %-8s L:%d U:%d",
            i,
            state_to_string(state[i]),
            digitalRead(SENSORS_LOWER[i]),
            digitalRead(SENSORS_UPPER[i])
        );
        strcat(line, buf);
        if (i < NUM_BARRELS - 1) strcat(line, " | ");
    }

    logger(line);
}

void step() {
    delay(100);
    log_state();
}


// Relay is low-triggered!
void close_valve(int valve) { digitalWrite(valve, HIGH); }
void open_valve(int valve) { digitalWrite(valve, LOW); }


void _init(int barrel) {
    logger("Init " + String(barrel));
    state[barrel] = INIT;

    open_valve(VALVES_WORK[barrel]);
    open_valve(VALVES_INTAKE[barrel]);

    while (water_over_upper_level(barrel)) step();

    close_valve(VALVES_WORK[barrel]);

    while (not_enough_pressure(barrel)) step();

    close_valve(VALVES_INTAKE[barrel]);

    state[barrel] = READY_FOR_WORK;
    log_state();
}


void setup() {
    Serial.begin(9600);

    for (int i = 0; i < NUM_BARRELS; i++) {
        pinMode(VALVES_INTAKE[i], OUTPUT);
        pinMode(VALVES_EXHAUST[i], OUTPUT);
        pinMode(VALVES_WORK[i], OUTPUT);

        pinMode(SENSORS_LOWER[i], INPUT);
        pinMode(SENSORS_UPPER[i], INPUT);

        close_valve(VALVES_INTAKE[i]);
        close_valve(VALVES_EXHAUST[i]);
        close_valve(VALVES_WORK[i]);
    }

    for (int i = 0; i < NUM_BARRELS; i++) _init(i);

    state[0] = WORK;
    logger("Cycle begins");
}


void loop() {
    for (int i = 0; i < NUM_BARRELS; i++) {
        switch (state[i]) {
            case WORK:
                if (water_over_lower_level(i))
                    open_valve(VALVES_WORK[i]);
                else {
                    close_valve(VALVES_WORK[i]);
                    state[i] = EXHAUST;
                }
                break;

            case EXHAUST:
                if (!water_over_upper_level(i))
                    open_valve(VALVES_EXHAUST[i]);
                else {
                    close_valve(VALVES_EXHAUST[i]);
                    state[i] = INTAKE;
                }
                break;

            case INTAKE:
                // TODO use pressure sensor
                open_valve(VALVES_INTAKE[i]);
                delay(1000);
                close_valve(VALVES_INTAKE[i]);
                state[i] = READY_FOR_WORK;
                break;

            case READY_FOR_WORK:
                if (state[(i + 1) % NUM_BARRELS] != WORK)
                    state[i] = WORK;
                break;
        }
    }

    step();
}
