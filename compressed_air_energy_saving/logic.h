#pragma once

// Forward declare millis() only for non-Arduino environments
#ifndef ARDUINO
unsigned long millis();
#endif

enum State { INTAKE, WORK, EXHAUST, WAIT_FOR_INTAKE, WAIT_FOR_WORK };

// Array to track the state of each barrel (sized for maximum)
extern State barrel_states[]; // Will be sized according to MAX_BARRELS

// Global manual override system
extern bool manual_mode;
extern State manual_states[];

// Utility function to convert state enum to string
inline const char* state_name(State state) {
  switch (state) {
    case INTAKE: return "INTAKE";
    case WORK: return "WORK";
    case EXHAUST: return "EXHAUST";
    case WAIT_FOR_INTAKE: return "WAIT_FOR_INTAKE";
    case WAIT_FOR_WORK: return "WAIT_FOR_WORK";
    default: return "UNKNOWN";
  }
}

bool pressurized_enough(int barrel_index);
bool water_below_lower_level(int barrel_index);
bool water_reached_upper_level(int barrel_index);
void open_valve(int valve);
void close_valve(int valve);

void logic();

// Timing and performance measurement functions
void init_timing_system();
void record_state_duration(int barrel_index, State from_state, unsigned long duration);
unsigned long get_average_duration(int barrel_index, State state);
void print_timing_stats();

// Startup assessment and recovery functions
void assess_startup_state();
State determine_barrel_state_from_sensors(int barrel_index);
void safe_barrel_recovery(int barrel_index, State assessed_state);
void log_startup_assessment();
