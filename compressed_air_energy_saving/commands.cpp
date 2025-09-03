#include "constants.h"
#include "logic.h"
#include "commands.h"

// Parse and execute manual override commands (shared by both WiFi and Serial)
void process_command(String command) {
  command.trim();
  command.toUpperCase();

  if (command.startsWith("MODE ")) {
    String mode_str = command.substring(5);  // Remove "MODE "
    mode_str.trim();

    if (mode_str == "MANUAL") {
      // Preserve current barrel states when switching to manual mode
      for (int i = 0; i < NUM_BARRELS; i++) {
        manual_states[i] = barrel_states[i];
      }
      manual_mode = true;
      logger(F("System set to MANUAL mode - automatic logic disabled"));
      logger(F("Current barrel states preserved for manual control"));
      logger(F("Use CMD <STATE> <BARREL> to control individual barrels"));
    } else if (mode_str == "AUTO") {
      manual_mode = false;
      logger(F("System set to AUTO mode - automatic logic enabled"));

      // Assess and recover barrel states when switching to AUTO
      // This handles potentially messy states from manual mode
      logger(F("Assessing barrel states for safe automatic operation..."));
      assess_startup_state();
    } else {
      logger(F("ERROR: Invalid mode. Use: MODE AUTO or MODE MANUAL"));
    }
    return;
  }

  if (command.startsWith("CMD ")) {
    // Automatically switch to manual mode if not already in manual mode
    if (!manual_mode) {
      // Preserve current barrel states when auto-switching to manual
      for (int i = 0; i < NUM_BARRELS; i++) {
        manual_states[i] = barrel_states[i];
      }
      manual_mode = true;
      logger(F("Auto-switching to MANUAL mode for direct barrel control"));
    }

    String params = command.substring(4);  // Remove "CMD "
    int firstSpace = params.indexOf(' ');

    if (firstSpace == -1) {
      logger(F("ERROR: Invalid command format. Use: CMD <STATE> <BARREL>"));
      return;
    }

    String state_str = params.substring(0, firstSpace);
    String barrel_str = params.substring(firstSpace + 1);
    int barrel_num = barrel_str.toInt();

    // Validate barrel number
    if (barrel_num < 0 || barrel_num >= NUM_BARRELS) {
      logger("ERROR: Invalid barrel number. Use 0-" + String(NUM_BARRELS-1));
      return;
    }

    // Parse state
    State new_state;
    bool valid_state = true;
    if (state_str == "IDLE") {
      new_state = IDLE;
    } else if (state_str == "INIT") {
      new_state = INIT;
    } else if (state_str == "INTAKE") {
      new_state = INTAKE;
    } else if (state_str == "WORK") {
      new_state = WORK;
    } else if (state_str == "EXHAUST") {
      new_state = EXHAUST;
    } else if (state_str == "EXIT") {
      new_state = EXIT;
    } else if (state_str == "WAIT_INTAKE" || state_str == "WAIT_FOR_INTAKE") {
      new_state = WAIT_FOR_INTAKE;
    } else if (state_str == "WAIT_WORK" || state_str == "WAIT_FOR_WORK") {
      new_state = WAIT_FOR_WORK;
    } else {
      valid_state = false;
    }

    if (!valid_state) {
      logger(F("ERROR: Invalid state. Use: IDLE, INIT, INTAKE, WORK, EXHAUST, EXIT, WAIT_INTAKE, WAIT_WORK"));
      return;
    }

    // Apply manual state
    manual_states[barrel_num] = new_state;
    logger("Manual command: Barrel" + String(barrel_num) + " set to " + String(state_name(new_state)));

  } else if (command.startsWith("VALVE ")) {
    // Individual valve control: VALVE <BARREL> <VALVE> <OPEN|CLOSE>
    // Automatically switch to manual mode if needed
    if (!manual_mode) {
      for (int i = 0; i < NUM_BARRELS; i++) {
        manual_states[i] = barrel_states[i];
      }
      manual_mode = true;
      logger(F("Auto-switching to MANUAL mode for direct valve control"));
    }

    String params = command.substring(6);  // Remove "VALVE "
    int firstSpace = params.indexOf(' ');
    int secondSpace = params.indexOf(' ', firstSpace + 1);

    if (firstSpace == -1 || secondSpace == -1) {
      logger(F("ERROR: Invalid valve command format. Use: VALVE <BARREL> <VALVE> <OPEN|CLOSE>"));
      logger(F("  Valves: INTAKE, WORK, EXHAUST"));
      return;
    }

    String barrel_str = params.substring(0, firstSpace);
    String valve_str = params.substring(firstSpace + 1, secondSpace);
    String action_str = params.substring(secondSpace + 1);

    int barrel_num = barrel_str.toInt();
    valve_str.trim();
    action_str.trim();

    // Validate barrel number
    if (barrel_num < 0 || barrel_num >= NUM_BARRELS) {
      logger("ERROR: Invalid barrel number. Use 0-" + String(NUM_BARRELS-1));
      return;
    }

    // Determine valve pin
    int valve_pin = -1;
    if (valve_str == "INTAKE") {
      valve_pin = VALVES_INTAKE[barrel_num];
    } else if (valve_str == "WORK" || valve_str == "TURBINE") {
      valve_pin = VALVES_TO_TURBINE[barrel_num];
    } else if (valve_str == "EXHAUST") {
      valve_pin = VALVES_EXHAUST[barrel_num];
    } else {
      logger(F("ERROR: Invalid valve name. Use: INTAKE, WORK, EXHAUST"));
      return;
    }

    // Execute valve action
    if (action_str == "OPEN") {
      open_valve(valve_pin);
      logger("Manual valve control: Barrel" + String(barrel_num) + " " + valve_str + " valve OPENED");
    } else if (action_str == "CLOSE") {
      close_valve(valve_pin);
      logger("Manual valve control: Barrel" + String(barrel_num) + " " + valve_str + " valve CLOSED");
    } else {
      logger(F("ERROR: Invalid valve action. Use: OPEN or CLOSE"));
      return;
    }

  } else if (command.startsWith("SET ")) {
    // Configuration commands: SET <PARAMETER> <VALUE>
    String params = command.substring(4);  // Remove "SET "
    int spaceIndex = params.indexOf(' ');

    if (spaceIndex == -1) {
      logger(F("ERROR: Invalid SET command format. Use: SET <PARAMETER> <VALUE>"));
      return;
    }

    String param_str = params.substring(0, spaceIndex);
    String value_str = params.substring(spaceIndex + 1);
    param_str.trim();
    value_str.trim();

    if (param_str == "INTAKE_DURATION") {
      unsigned long new_duration = value_str.toInt();
      if (new_duration < 100 || new_duration > 10000) {
        logger(F("ERROR: Invalid INTAKE_DURATION. Use range 100-10000 ms"));
        return;
      }
      INTAKE_DURATION_MS = new_duration;
      logger("INTAKE duration set to " + String(INTAKE_DURATION_MS) + " ms");
    } else if (param_str == "NUM_BARRELS") {
      int new_num_barrels = value_str.toInt();
      if (new_num_barrels < 1 || new_num_barrels > MAX_BARRELS) {
        logger("ERROR: Invalid NUM_BARRELS. Use range 1-" + String(MAX_BARRELS));
        return;
      }
      NUM_BARRELS = new_num_barrels;
      logger("Number of barrels set to " + String(NUM_BARRELS));
    } else {
      logger(F("ERROR: Unknown parameter. Available: INTAKE_DURATION, NUM_BARRELS"));
    }

  } else if (command == "STATUS") {
    logger(F("=== SYSTEM STATUS ==="));
    logger("Number of barrels: " + String(NUM_BARRELS));
    logger("INTAKE duration limit: " + String(INTAKE_DURATION_MS) + " ms");
    if (manual_mode) {
      logger(F("Mode: MANUAL"));
      for (int i = 0; i < NUM_BARRELS; i++) {
        logger("Barrel" + String(i) + ": " + String(state_name(manual_states[i])) + " (manual)");
      }
    } else {
      logger(F("Mode: AUTO"));
      for (int i = 0; i < NUM_BARRELS; i++) {
        logger("Barrel" + String(i) + ": " + String(state_name(barrel_states[i])) + " (auto)");
      }
    }
  } else if (command == "HELP") {
    logger(F("=== AVAILABLE COMMANDS ==="));
    logger(F("MODE <AUTO|MANUAL> - Switch between automatic and manual control"));
    logger(F("CMD <STATE> <BARREL> - Set barrel state (auto-switches to manual mode)"));
    logger(F("  States: IDLE, INIT, INTAKE, WORK, EXHAUST, EXIT, WAIT_INTAKE, WAIT_WORK"));
    logger(F("  Example: CMD INTAKE 0"));
    logger(F("VALVE <BARREL> <VALVE> <OPEN|CLOSE> - Control individual valves"));
    logger(F("  Valves: INTAKE, WORK, EXHAUST"));
    logger(F("  Example: VALVE 0 INTAKE OPEN"));
    logger(F("SET <PARAMETER> <VALUE> - Configure system parameters"));
    logger(F("  INTAKE_DURATION <ms> - Set INTAKE time limit (100-10000 ms)"));
    logger(F("  NUM_BARRELS <count> - Set number of active barrels (1-4)"));
    logger(F("  Example: SET INTAKE_DURATION 3000"));
    logger(F("STATUS - Show current system and barrel status"));
    logger(F("HELP - Show this help"));
  } else {
    logger("Unknown command: " + command + ". Type HELP for available commands.");
  }
}
