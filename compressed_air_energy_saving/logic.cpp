#include "constants.h"
#include "logic.h"

// Define the number of barrels (can be changed dynamically)
int NUM_BARRELS = 2;

// Initialize barrel states - all start in WAIT (system will choose one to start)
State barrel_states[MAX_BARRELS] = {WAIT, WAIT, WAIT, WAIT};

// Check if all barrels are in WAIT state (system startup scenario)
bool all_barrels_waiting() {
  for (int i = 0; i < NUM_BARRELS; i++) {
    if (barrel_states[i] != WAIT) {
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

void handle_barrel_logic(int barrel_index, bool is_startup = false) {
  switch (barrel_states[barrel_index]) {
    case INTAKE:
      if (pressurized_enough(barrel_index)) {
        close_valve(VALVES_INTAKE[barrel_index]);
        // Only transition to WORK if no other barrel is working
        if (!any_other_barrel_working(barrel_index)) {
          barrel_states[barrel_index] = WORK;
          // Immediately start working
          open_valve(VALVES_TO_TURBINE[barrel_index]);
        } else {
          barrel_states[barrel_index] = WAIT;
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
        close_valve(VALVES_TO_TURBINE[barrel_index]);
        barrel_states[barrel_index] = EXHAUST;
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
        close_valve(VALVES_EXHAUST[barrel_index]);
        barrel_states[barrel_index] = INTAKE;
        // Don't immediately open intake - wait for pressure check
      } else {
        open_valve(VALVES_EXHAUST[barrel_index]);
      }
      // Ensure other valves are closed
      close_valve(VALVES_INTAKE[barrel_index]);
      close_valve(VALVES_TO_TURBINE[barrel_index]);
      break;

    case WAIT:
      // Waiting for other barrels to finish WORK
      if (!any_other_barrel_active(barrel_index)) {
        // If all barrels are in WAIT (system startup), only barrel 0 starts
        if (is_startup) {
          if (barrel_index == 0) {
            barrel_states[barrel_index] = INTAKE;
          }
          // Other barrels stay in WAIT
        } else {
          // Normal case: no other barrel is active, this barrel can start
          barrel_states[barrel_index] = INTAKE;
        }
      }
      // Keep all valves closed while waiting
      close_valve(VALVES_INTAKE[barrel_index]);
      close_valve(VALVES_EXHAUST[barrel_index]);
      close_valve(VALVES_TO_TURBINE[barrel_index]);
      break;
  }
}

void logic() {
  // Check if this is system startup (all barrels in WAIT)
  bool is_startup = all_barrels_waiting();

  // Process logic for each barrel
  for (int i = 0; i < NUM_BARRELS; i++) {
    handle_barrel_logic(i, is_startup);
  }
}
