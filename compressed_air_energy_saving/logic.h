#pragma once

enum State { INTAKE, WORK, EXHAUST, WAIT };

// Array to track the state of each barrel (sized for maximum)
extern State barrel_states[]; // Will be sized according to MAX_BARRELS

// Utility function to convert state enum to string
inline const char* state_name(State state) {
  switch (state) {
    case INTAKE: return "INTAKE";
    case WORK: return "WORK";
    case EXHAUST: return "EXHAUST";
    case WAIT: return "WAIT";
    default: return "UNKNOWN";
  }
}

bool pressurized_enough(int barrel_index);
bool water_below_lower_level(int barrel_index);
bool water_reached_upper_level(int barrel_index);
void open_valve(int valve);
void close_valve(int valve);

void logic();
