#include <iostream>
#include <cassert>
#include <map>

#include "constants.h"
#include "logic.h"

bool mock_pressurized[MAX_BARRELS] = {false};
bool mock_water_below[MAX_BARRELS] = {false};
bool mock_water_upper[MAX_BARRELS] = {false};

// Track valve states (starts with all valves closed)
std::map<int, bool> valve_states;

// Initialize valve states for all barrels up to MAX_BARRELS
void init_valve_states() {
    valve_states.clear();
    for (int i = 0; i < MAX_BARRELS; i++) {
        valve_states[VALVES_INTAKE[i]] = false;
        valve_states[VALVES_EXHAUST[i]] = false;
        valve_states[VALVES_TO_TURBINE[i]] = false;
    }
}

bool pressurized_enough(int barrel_index) {
    return mock_pressurized[barrel_index];
}
bool water_below_lower_level(int barrel_index) {
    return mock_water_below[barrel_index];
}
bool water_reached_upper_level(int barrel_index) {
    return mock_water_upper[barrel_index];
}

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

void assert_barrel_state(int barrel_index, State expected, const std::string& message) {
    assert(barrel_states[barrel_index] == expected);
    std::cout << "  ✓ " << message << std::endl;
}

void print_barrel_states() {
    std::cout << "  States: ";
    for (int i = 0; i < NUM_BARRELS; i++) {
        std::cout << "Barrel" << i << ":" << state_name(barrel_states[i]);
        if (i < NUM_BARRELS - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

void reset_test_state() {
    // Reset all mock states
    for (int i = 0; i < MAX_BARRELS; i++) {
        mock_pressurized[i] = false;
        mock_water_below[i] = false;
        mock_water_upper[i] = false;
        barrel_states[i] = WAIT; // All barrels start in WAIT
    }
}

void test_single_barrel_cycle() {
    std::cout << "\n=== Testing single barrel cycle (NUM_BARRELS=1) ===" << std::endl;

    // Set up for single barrel test
    NUM_BARRELS = 1;
    init_valve_states();
    reset_test_state();

    std::cout << "0. System startup: Barrel starts in WAIT, system chooses it to start..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 transitions from WAIT to INTAKE (chosen to start)");

    std::cout << "1. Starting in INTAKE state with low pressure..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 stays in INTAKE (low pressure)");
    assert_valve_state(VALVES_INTAKE[0], true, "Intake valve opened for air intake");
    assert_valve_state(VALVES_EXHAUST[0], false, "Exhaust valve remains closed");
    assert_valve_state(VALVES_TO_TURBINE[0], false, "Turbine valve remains closed");

    std::cout << "2. Pressure reaches target, transitioning to WORK..." << std::endl;
    mock_pressurized[0] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, WORK, "Barrel0 transitions to WORK");
    assert_valve_state(VALVES_INTAKE[0], false, "Intake valve closed (pressure reached)");
    assert_valve_state(VALVES_EXHAUST[0], false, "Exhaust valve still closed");
    assert_valve_state(VALVES_TO_TURBINE[0], true, "Turbine valve opened for power generation");

    std::cout << "3. Water drops below lower level, transitioning to EXHAUST..." << std::endl;
    mock_water_below[0] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 transitions to EXHAUST");
    assert_valve_state(VALVES_INTAKE[0], false, "Intake valve still closed");
    assert_valve_state(VALVES_EXHAUST[0], true, "Exhaust valve opened for pressure release");
    assert_valve_state(VALVES_TO_TURBINE[0], false, "Turbine valve closed (stopped power generation)");

    std::cout << "4. Water reaches upper level, returning to INTAKE..." << std::endl;
    mock_water_upper[0] = true;
    mock_water_below[0] = false;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 transitions back to INTAKE");
    assert_valve_state(VALVES_INTAKE[0], false, "Intake valve still closed (no pressure yet)");
    assert_valve_state(VALVES_EXHAUST[0], false, "Exhaust valve closed (cycle complete)");
    assert_valve_state(VALVES_TO_TURBINE[0], false, "Turbine valve still closed");

    std::cout << "=== Single barrel cycle test passed! ===" << std::endl;
}

void test_multi_barrel_coordination() {
    std::cout << "\n=== Testing multi-barrel coordination (NUM_BARRELS=2) ===" << std::endl;

    // Set up for multi-barrel test
    NUM_BARRELS = 2;
    init_valve_states();
    reset_test_state();

    std::cout << "0. System startup: All barrels start in WAIT, system chooses Barrel0..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 chosen to start (transitions from WAIT to INTAKE)");
    assert_barrel_state(1, WAIT, "Barrel1 remains in WAIT");

    std::cout << "1. Barrel0 starts intake, Barrel1 waits..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 stays in INTAKE (low pressure)");
    assert_barrel_state(1, WAIT, "Barrel1 stays in WAIT");
    assert_valve_state(VALVES_INTAKE[0], true, "Barrel0 intake valve open");
    assert_valve_state(VALVES_INTAKE[1], false, "Barrel1 intake valve closed (waiting)");

    std::cout << "2. Barrel0 reaches pressure and starts working..." << std::endl;
    mock_pressurized[0] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, WORK, "Barrel0 transitions to WORK");
    assert_barrel_state(1, WAIT, "Barrel1 stays in WAIT");
    assert_valve_state(VALVES_TO_TURBINE[0], true, "Barrel0 turbine valve open");
    assert_valve_state(VALVES_INTAKE[0], false, "Barrel0 intake valve closed");

    std::cout << "3. Barrel0 water drops, transitions to EXHAUST..." << std::endl;
    mock_water_below[0] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 transitions to EXHAUST");
    assert_barrel_state(1, INTAKE, "Barrel1 transitions from WAIT to INTAKE");
    assert_valve_state(VALVES_TO_TURBINE[0], false, "Barrel0 turbine valve closed");
    assert_valve_state(VALVES_EXHAUST[0], true, "Barrel0 exhaust valve opened");

    std::cout << "4. Barrel1 starts intake process..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 continues in EXHAUST");
    assert_barrel_state(1, INTAKE, "Barrel1 continues in INTAKE");
    assert_valve_state(VALVES_INTAKE[1], true, "Barrel1 intake valve opened");

    std::cout << "5. Barrel1 reaches pressure and starts working..." << std::endl;
    mock_pressurized[1] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 continues in EXHAUST");
    assert_barrel_state(1, WORK, "Barrel1 transitions to WORK");
    assert_valve_state(VALVES_TO_TURBINE[1], true, "Barrel1 turbine valve opened");
    assert_valve_state(VALVES_INTAKE[1], false, "Barrel1 intake valve closed");

    std::cout << "6. Barrel0 completes cycle, Barrel1 continues working..." << std::endl;
    mock_water_upper[0] = true;
    mock_water_below[0] = false;
    mock_pressurized[0] = false;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 transitions to INTAKE");
    assert_barrel_state(1, WORK, "Barrel1 continues in WORK");
    assert_valve_state(VALVES_EXHAUST[0], false, "Barrel0 exhaust valve closed");
    assert_valve_state(VALVES_TO_TURBINE[1], true, "Barrel1 turbine valve still open");

    std::cout << "=== Multi-barrel coordination test passed! ===" << std::endl;
}

int main() {
    test_single_barrel_cycle();
    test_multi_barrel_coordination();
    return 0;
}
