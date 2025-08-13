#include <iostream>
#include <cassert>
#include <map>

#include "constants.h"
#include "logic.h"

bool mock_pressurized = false;
bool mock_water_below = false;
bool mock_water_upper = false;

// Track valve states (starts with all valves closed)
std::map<int, bool> valve_states = {
    {VALVE_INTAKE, false},
    {VALVE_EXHAUST, false},
    {VALVE_TO_TURBINE, false}
};

bool pressurized_enough() { return mock_pressurized; }
bool water_below_lower_level() { return mock_water_below; }
bool water_reached_upper_level() { return mock_water_upper; }

void open_valve(int valve) {
    valve_states[valve] = true;
    std::cout << "  Opened valve " << valve << std::endl;
}

void close_valve(int valve) {
    valve_states[valve] = false;
    std::cout << "  Closed valve " << valve << std::endl;
}

void assert_valve_state(int valve, bool expected, const std::string& message) {
    assert(valve_states[valve] == expected);
    std::cout << "  ✓ " << message << std::endl;
}

void test_realistic_sequence() {
    std::cout << "=== Testing realistic operation sequence ===" << std::endl;

    std::cout << "1. Starting in INTAKE state with low pressure..." << std::endl;
    current_state = INTAKE;
    mock_pressurized = false;
    mock_water_below = false;
    mock_water_upper = false;
    logic();
    assert(current_state == INTAKE);
    assert_valve_state(VALVE_INTAKE, true, "VALVE_INTAKE opened for air intake");
    assert_valve_state(VALVE_EXHAUST, false, "VALVE_EXHAUST remains closed");
    assert_valve_state(VALVE_TO_TURBINE, false, "VALVE_TO_TURBINE remains closed");

    std::cout << "2. Pressure reaches target, transitioning to WORK..." << std::endl;
    mock_pressurized = true;
    logic();
    assert(current_state == WORK);
    assert_valve_state(VALVE_INTAKE, false, "VALVE_INTAKE closed (pressure reached)");
    assert_valve_state(VALVE_EXHAUST, false, "VALVE_EXHAUST still closed");
    assert_valve_state(VALVE_TO_TURBINE, false, "VALVE_TO_TURBINE still closed");

    std::cout << "3. In WORK state, generating power..." << std::endl;
    mock_pressurized = false; // doesn't matter in WORK state
    mock_water_below = false; // water above lower level
    logic();
    assert(current_state == WORK);
    assert_valve_state(VALVE_INTAKE, false, "VALVE_INTAKE still closed");
    assert_valve_state(VALVE_EXHAUST, false, "VALVE_EXHAUST still closed");
    assert_valve_state(VALVE_TO_TURBINE, true, "VALVE_TO_TURBINE opened for power generation");

    std::cout << "4. Water drops below lower level, transitioning to EXHAUST..." << std::endl;
    mock_water_below = true;
    logic();
    assert(current_state == EXHAUST);
    assert_valve_state(VALVE_INTAKE, false, "VALVE_INTAKE still closed");
    assert_valve_state(VALVE_EXHAUST, false, "VALVE_EXHAUST still closed");
    assert_valve_state(VALVE_TO_TURBINE, false, "VALVE_TO_TURBINE closed (stopped power generation)");

    std::cout << "5. In EXHAUST state, releasing pressure..." << std::endl;
    mock_water_below = false; // doesn't matter in EXHAUST state
    mock_water_upper = false; // water below upper level
    logic();
    assert(current_state == EXHAUST);
    assert_valve_state(VALVE_INTAKE, false, "VALVE_INTAKE still closed");
    assert_valve_state(VALVE_EXHAUST, true, "VALVE_EXHAUST opened for pressure release");
    assert_valve_state(VALVE_TO_TURBINE, false, "VALVE_TO_TURBINE still closed");

    std::cout << "6. Water reaches upper level, returning to INTAKE..." << std::endl;
    mock_water_upper = true;
    logic();
    assert(current_state == INTAKE);
    assert_valve_state(VALVE_INTAKE, false, "VALVE_INTAKE still closed");
    assert_valve_state(VALVE_EXHAUST, false, "VALVE_EXHAUST closed (cycle complete)");
    assert_valve_state(VALVE_TO_TURBINE, false, "VALVE_TO_TURBINE still closed");

    std::cout << "=== Complete test cycle passed! ===" << std::endl;
}

int main() {
    test_realistic_sequence();
    return 0;
}
