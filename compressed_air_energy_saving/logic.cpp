#ifdef ARDUINO
#include <Arduino.h>
#endif
#include "constants.h"
#include "logic.h"

// Define the number of barrels (can be changed dynamically)
int NUM_BARRELS = 1;

// Initialize barrel states - all start in WAIT_FOR_INTAKE (system will choose ones to start)
State barrel_states[MAX_BARRELS] = {WAIT_FOR_INTAKE, WAIT_FOR_INTAKE, WAIT_FOR_INTAKE, WAIT_FOR_INTAKE};

// Global manual override system
bool manual_mode = false;
State manual_states[MAX_BARRELS] = {WAIT_FOR_INTAKE, WAIT_FOR_INTAKE, WAIT_FOR_INTAKE, WAIT_FOR_INTAKE};

// Initialize barrel timers - track when each barrel entered its current state
unsigned long barrel_timers[MAX_BARRELS] = {0, 0, 0, 0};

// Timing system - scale history size based on available memory
#ifdef ARDUINO_UNOR4_WIFI
  #define TIMING_HISTORY_SIZE 10  // R4 WiFi: 32KB RAM - full history
#else
  #define TIMING_HISTORY_SIZE 3   // R3 Uno: 2KB RAM - minimal history
#endif

// Timing data using simple arrays for Arduino compatibility
// [barrel][state][history_index] - state index: 0=INTAKE, 1=WORK, 2=EXHAUST, 3=WAIT_FOR_INTAKE, 4=WAIT_FOR_WORK
unsigned long timing_history[MAX_BARRELS][5][TIMING_HISTORY_SIZE];
unsigned long current_durations[MAX_BARRELS][5]; // Current cycle durations
int history_index[MAX_BARRELS] = {0, 0, 0, 0}; // Current index in circular buffer

// Initialize timing system
void init_timing_system() {
  for (int i = 0; i < MAX_BARRELS; i++) {
    for (int state = 0; state < 5; state++) {
      for (int j = 0; j < TIMING_HISTORY_SIZE; j++) {
        timing_history[i][state][j] = 0;
      }
      current_durations[i][state] = 0;
    }
    history_index[i] = 0;
    barrel_timers[i] = 0;
  }
}

// Helper function to record state duration and update history
void record_state_duration(int barrel_index, State from_state, unsigned long duration) {
  if (barrel_index < 0 || barrel_index >= MAX_BARRELS) return; // Bounds check

  int idx = history_index[barrel_index];

  // Record current duration (using enum value directly as array index)
  current_durations[barrel_index][from_state] = duration;

  // Add to history
  timing_history[barrel_index][from_state][idx] = duration;

  // Advance circular buffer index (shared across all states for this barrel)
  history_index[barrel_index] = (idx + 1) % TIMING_HISTORY_SIZE;
}

// Calculate average duration for a state across recent history
unsigned long get_average_duration(int barrel_index, State state) {
  if (barrel_index < 0 || barrel_index >= MAX_BARRELS) return 0; // Bounds check

  unsigned long total = 0;
  int count = 0;

  // Use enum value directly as array index
  for (int i = 0; i < TIMING_HISTORY_SIZE; i++) {
    if (timing_history[barrel_index][state][i] > 0) {
      total += timing_history[barrel_index][state][i];
      count++;
    }
  }

  return count > 0 ? total / count : 0;
}// Print timing statistics for all barrels
void print_timing_stats() {
  for (int i = 0; i < NUM_BARRELS; i++) {
    unsigned long avg_intake = get_average_duration(i, INTAKE);
    unsigned long avg_work = get_average_duration(i, WORK);
    unsigned long avg_exhaust = get_average_duration(i, EXHAUST);
    unsigned long avg_wait_intake = get_average_duration(i, WAIT_FOR_INTAKE);
    unsigned long avg_wait_work = get_average_duration(i, WAIT_FOR_WORK);

    if (avg_intake > 0 || avg_work > 0 || avg_exhaust > 0 || avg_wait_intake > 0 || avg_wait_work > 0) {
      // This would be Serial.print in real Arduino
      // For now, keeping it simple for testing
    }
  }
}

// Count how many barrels are in INTAKE state
int count_barrels_in_intake() {
  int count = 0;
  for (int i = 0; i < NUM_BARRELS; i++) {
    if (barrel_states[i] == INTAKE) {
      count++;
    }
  }
  return count;
}

// Count how many barrels are in WORK state
int count_barrels_in_work() {
  int count = 0;
  for (int i = 0; i < NUM_BARRELS; i++) {
    if (barrel_states[i] == WORK) {
      count++;
    }
  }
  return count;
}

// Apply valve actions for a given state (used by manual override)
void apply_valve_actions(int barrel_index, State state) {
  switch (state) {
    case INTAKE:
      open_valve(VALVES_INTAKE[barrel_index]);
      close_valve(VALVES_EXHAUST[barrel_index]);
      close_valve(VALVES_TO_TURBINE[barrel_index]);
      break;
    case WORK:
      close_valve(VALVES_INTAKE[barrel_index]);
      close_valve(VALVES_EXHAUST[barrel_index]);
      open_valve(VALVES_TO_TURBINE[barrel_index]);
      break;
    case EXHAUST:
      close_valve(VALVES_INTAKE[barrel_index]);
      open_valve(VALVES_EXHAUST[barrel_index]);
      close_valve(VALVES_TO_TURBINE[barrel_index]);
      break;
    case WAIT_FOR_INTAKE:
    case WAIT_FOR_WORK:
      close_valve(VALVES_INTAKE[barrel_index]);
      close_valve(VALVES_EXHAUST[barrel_index]);
      close_valve(VALVES_TO_TURBINE[barrel_index]);
      break;
  }
}

// Check if any barrel is currently in INTAKE state
bool any_barrel_in_intake() {
  return count_barrels_in_intake() > 0;
}

// Check if we're in startup phase (no barrel has ever worked yet)
bool is_startup_phase() {
  for (int i = 0; i < NUM_BARRELS; i++) {
    if (barrel_states[i] == WORK) {
      return false;
    }
  }
  return true;
}

// Check if any barrel is currently in WORK or INTAKE state (excluding the given barrel index)
bool any_other_barrel_active(int current_barrel) {
  for (int i = 0; i < NUM_BARRELS; i++) {
    if (i != current_barrel && (barrel_states[i] == WORK || barrel_states[i] == INTAKE)) {
      return true;
    }
  }
  return false;
}
bool any_other_barrel_working(int current_barrel) {
  for (int i = 0; i < NUM_BARRELS; i++) {
    if (i != current_barrel && barrel_states[i] == WORK) {
      return true;
    }
  }
  return false;
}

// Check if any barrel is currently in WORK state (including all barrels)
bool any_barrel_working() {
  for (int i = 0; i < NUM_BARRELS; i++) {
    if (barrel_states[i] == WORK) {
      return true;
    }
  }
  return false;
}

void handle_barrel_logic(int barrel_index) {
  State current_state = barrel_states[barrel_index];
  unsigned long current_time = millis();

  switch (current_state) {
    case INTAKE:
      if (pressurized_enough(barrel_index)) {
        // Record INTAKE duration before transitioning
        unsigned long intake_duration = current_time - barrel_timers[barrel_index];
        record_state_duration(barrel_index, INTAKE, intake_duration);

        close_valve(VALVES_INTAKE[barrel_index]);
        // Only transition to WORK if no other barrel is working
        if (!any_other_barrel_working(barrel_index)) {
          barrel_states[barrel_index] = WORK;
          barrel_timers[barrel_index] = current_time; // Record WORK start time
          // Immediately start working
          open_valve(VALVES_TO_TURBINE[barrel_index]);
        } else {
          barrel_states[barrel_index] = WAIT_FOR_WORK;
          barrel_timers[barrel_index] = current_time; // Record WAIT_FOR_WORK start time
        }
      } else {
        open_valve(VALVES_INTAKE[barrel_index]);
      }
      // Ensure other valves are closed
      if (barrel_states[barrel_index] != WORK) {
        close_valve(VALVES_TO_TURBINE[barrel_index]);
      }
      close_valve(VALVES_EXHAUST[barrel_index]);
      break;

    case WORK:
      if (water_below_lower_level(barrel_index)) {
        // Record WORK duration before transitioning
        unsigned long work_duration = current_time - barrel_timers[barrel_index];
        record_state_duration(barrel_index, WORK, work_duration);

        close_valve(VALVES_TO_TURBINE[barrel_index]);
        barrel_states[barrel_index] = EXHAUST;
        barrel_timers[barrel_index] = current_time; // Record EXHAUST start time
        // Immediately start exhausting
        open_valve(VALVES_EXHAUST[barrel_index]);
      } else {
        open_valve(VALVES_TO_TURBINE[barrel_index]);
      }
      // Ensure other valves are closed
      close_valve(VALVES_INTAKE[barrel_index]);
      if (barrel_states[barrel_index] != EXHAUST) {
        close_valve(VALVES_EXHAUST[barrel_index]);
      }
      break;

    case EXHAUST:
      if (water_reached_upper_level(barrel_index)) {
        // Record EXHAUST duration before transitioning
        unsigned long exhaust_duration = current_time - barrel_timers[barrel_index];
        record_state_duration(barrel_index, EXHAUST, exhaust_duration);

        close_valve(VALVES_EXHAUST[barrel_index]);

        // For single barrel system, go directly to INTAKE
        if (NUM_BARRELS == 1) {
          barrel_states[barrel_index] = INTAKE;
          barrel_timers[barrel_index] = current_time; // Record INTAKE start time
        } else if (NUM_BARRELS == 2) {
          // For 2-barrel system: always go directly to INTAKE for continuous production
          // One barrel should always be preparing while the other works
          barrel_states[barrel_index] = INTAKE;
          barrel_timers[barrel_index] = current_time;
          open_valve(VALVES_INTAKE[barrel_index]);
        } else {
          // For 3+ barrels: use more complex coordination
          // If no barrel is working and no barrel is preparing, start preparing immediately
          if (count_barrels_in_work() == 0 && count_barrels_in_intake() == 0) {
            barrel_states[barrel_index] = INTAKE;
            barrel_timers[barrel_index] = current_time;
            open_valve(VALVES_INTAKE[barrel_index]);
          }
          // If no other barrel is preparing, start preparing for next cycle
          else if (count_barrels_in_intake() == 0) {
            barrel_states[barrel_index] = INTAKE;
            barrel_timers[barrel_index] = current_time;
            open_valve(VALVES_INTAKE[barrel_index]);
          } else {
            barrel_states[barrel_index] = WAIT_FOR_INTAKE;
            barrel_timers[barrel_index] = current_time;
          }
        }
      } else {
        open_valve(VALVES_EXHAUST[barrel_index]);
      }
      // Ensure other valves are closed (except intake if we just transitioned to INTAKE)
      if (barrel_states[barrel_index] != INTAKE) {
        close_valve(VALVES_INTAKE[barrel_index]);
      }
      close_valve(VALVES_TO_TURBINE[barrel_index]);
      break;

    case WAIT_FOR_INTAKE: {
      // For 2-barrel system: immediately start INTAKE if no other barrel is preparing
      // For 3+ barrels: use more selective logic
      bool should_start_intake = false;
      if (NUM_BARRELS == 2) {
        // 2-barrel: during startup allow both, during normal operation maintain exactly one barrel in INTAKE
        should_start_intake = (is_startup_phase() && count_barrels_in_intake() < 2) ||
                             (!is_startup_phase() && count_barrels_in_intake() == 0);
      } else {
        // 3+ barrels: startup allows multiple, normal operation is more selective
        should_start_intake = ((is_startup_phase() && count_barrels_in_intake() < 2) ||
                              (!is_startup_phase() && (count_barrels_in_intake() == 0 || count_barrels_in_work() == 0)));
      }

      if (should_start_intake) {
        // Record WAIT_FOR_INTAKE duration before transitioning
        unsigned long wait_duration = current_time - barrel_timers[barrel_index];
        record_state_duration(barrel_index, WAIT_FOR_INTAKE, wait_duration);

        barrel_states[barrel_index] = INTAKE;
        barrel_timers[barrel_index] = current_time; // Record INTAKE start time
      }
      // Keep all valves closed while waiting
      close_valve(VALVES_INTAKE[barrel_index]);
      close_valve(VALVES_EXHAUST[barrel_index]);
      close_valve(VALVES_TO_TURBINE[barrel_index]);
      break;
    } // end of WAIT_FOR_INTAKE case

    case WAIT_FOR_WORK:
      // Waiting for current working barrel to finish (this barrel is pressurized)
      if (!any_other_barrel_working(barrel_index)) {
        // Record WAIT_FOR_WORK duration before transitioning
        unsigned long wait_duration = current_time - barrel_timers[barrel_index];
        record_state_duration(barrel_index, WAIT_FOR_WORK, wait_duration);

        // No other barrel is working, can start immediately
        barrel_states[barrel_index] = WORK;
        barrel_timers[barrel_index] = current_time; // Record WORK start time
        open_valve(VALVES_TO_TURBINE[barrel_index]);
      }
      // Keep all valves closed while waiting
      close_valve(VALVES_INTAKE[barrel_index]);
      close_valve(VALVES_EXHAUST[barrel_index]);
      if (barrel_states[barrel_index] != WORK) {
        close_valve(VALVES_TO_TURBINE[barrel_index]);
      }
      break;
  }
}

void logic() {
  // Process logic for each barrel
  for (int i = 0; i < NUM_BARRELS; i++) {
    if (manual_mode) {
      // Manual mode: apply manual states directly
      if (barrel_states[i] != manual_states[i]) {
        // Record duration of previous state before manual change
        unsigned long state_duration = millis() - barrel_timers[i];
        record_state_duration(i, barrel_states[i], state_duration);

        // Apply manual state
        barrel_states[i] = manual_states[i];
        barrel_timers[i] = millis();

        // Apply appropriate valve actions for manual state
        apply_valve_actions(i, manual_states[i]);
      }
    } else {
      // Automatic mode: normal logic
      handle_barrel_logic(i);
    }
  }
}
