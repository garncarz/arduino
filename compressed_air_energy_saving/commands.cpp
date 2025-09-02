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
    if (state_str == "INTAKE") {
      new_state = INTAKE;
    } else if (state_str == "WORK") {
      new_state = WORK;
    } else if (state_str == "EXHAUST") {
      new_state = EXHAUST;
    } else if (state_str == "WAIT_INTAKE" || state_str == "WAIT_FOR_INTAKE") {
      new_state = WAIT_FOR_INTAKE;
    } else if (state_str == "WAIT_WORK" || state_str == "WAIT_FOR_WORK") {
      new_state = WAIT_FOR_WORK;
    } else {
      valid_state = false;
    }

    if (!valid_state) {
      logger(F("ERROR: Invalid state. Use: INTAKE, WORK, EXHAUST, WAIT_INTAKE, WAIT_WORK"));
      return;
    }

    // Apply manual state
    manual_states[barrel_num] = new_state;
    logger("Manual command: Barrel" + String(barrel_num) + " set to " + String(state_name(new_state)));

  } else if (command == "STATUS") {
    logger(F("=== SYSTEM STATUS ==="));
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
    logger(F("  States: INTAKE, WORK, EXHAUST, WAIT_INTAKE, WAIT_WORK"));
    logger(F("  Example: CMD INTAKE 0"));
    logger(F("STATUS - Show current system and barrel status"));
    logger(F("HELP - Show this help"));
  } else {
    logger("Unknown command: " + command + ". Type HELP for available commands.");
  }
}
