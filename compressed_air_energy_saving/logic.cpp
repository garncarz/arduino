#include "constants.h"
#include "logic.h"

State current_state = INTAKE;

void logic() {
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
}
