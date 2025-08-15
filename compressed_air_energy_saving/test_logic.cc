#include <iostream>
#include <cassert>
#include <map>

#include "constants.h"
#include "logic.h"

// Mock timing functions for testing
unsigned long mock_time = 0;
unsigned long millis() { return mock_time; }
void advance_time(unsigned long ms) { mock_time += ms; }

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
        barrel_states[i] = WAIT_FOR_INTAKE; // All barrels start in WAIT_FOR_INTAKE
    }
}

void test_single_barrel_cycle() {
    std::cout << "\n=== Testing single barrel cycle (NUM_BARRELS=1) ===" << std::endl;

    // Set up for single barrel test
    NUM_BARRELS = 1;
    init_valve_states();
    init_timing_system(); // Initialize timing system
    reset_test_state();

    std::cout << "0. System startup: Barrel starts in WAIT_FOR_INTAKE, system chooses it to start..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 transitions from WAIT_FOR_INTAKE to INTAKE (chosen to start)");

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

    std::cout << "0. System startup: All barrels start in WAIT_FOR_INTAKE, system chooses first two..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 chosen to start (transitions from WAIT_FOR_INTAKE to INTAKE)");
    assert_barrel_state(1, INTAKE, "Barrel1 also starts INTAKE (parallel preparation)");

    std::cout << "1. Both barrels in intake, building pressure..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 stays in INTAKE (low pressure)");
    assert_barrel_state(1, INTAKE, "Barrel1 stays in INTAKE (low pressure)");
    assert_valve_state(VALVES_INTAKE[0], true, "Barrel0 intake valve open");
    assert_valve_state(VALVES_INTAKE[1], true, "Barrel1 intake valve open");

    std::cout << "2. Barrel0 reaches pressure first and starts working..." << std::endl;
    mock_pressurized[0] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, WORK, "Barrel0 transitions to WORK (first to reach pressure)");
    assert_barrel_state(1, INTAKE, "Barrel1 continues INTAKE (preparing for handoff)");
    assert_valve_state(VALVES_TO_TURBINE[0], true, "Barrel0 turbine valve open");
    assert_valve_state(VALVES_INTAKE[0], false, "Barrel0 intake valve closed");
    assert_valve_state(VALVES_INTAKE[1], true, "Barrel1 intake valve open (still preparing)");

    std::cout << "3. Barrel1 continues preparing while Barrel0 works..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, WORK, "Barrel0 continues working");
    assert_barrel_state(1, INTAKE, "Barrel1 continues preparing");
    assert_valve_state(VALVES_INTAKE[1], true, "Barrel1 intake valve still open");

    std::cout << "4. Barrel0 water drops, Barrel1 should immediately take over..." << std::endl;
    mock_water_below[0] = true;
    mock_pressurized[1] = true; // Barrel1 has had time to pressurize
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 transitions to EXHAUST");
    assert_barrel_state(1, WORK, "Barrel1 immediately transitions to WORK (was ready)");
    assert_valve_state(VALVES_TO_TURBINE[0], false, "Barrel0 turbine valve closed");
    assert_valve_state(VALVES_EXHAUST[0], true, "Barrel0 exhaust valve opened");
    assert_valve_state(VALVES_TO_TURBINE[1], true, "Barrel1 turbine valve opened");
    assert_valve_state(VALVES_INTAKE[1], false, "Barrel1 intake valve closed");

    std::cout << "5. Barrel1 working, Barrel0 exhausting - no energy gap!" << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 continues in EXHAUST");
    assert_barrel_state(1, WORK, "Barrel1 continues in WORK (continuous energy)");

    std::cout << "6. Barrel0 completes cycle, starts preparing for next handoff..." << std::endl;
    std::cout << "5. Barrel0 completes cycle, starts preparing for next handoff..." << std::endl;
    mock_water_upper[0] = true;
    mock_water_below[0] = false;
    mock_pressurized[0] = false;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 transitions to INTAKE (preparing for next cycle)");
    assert_barrel_state(1, WORK, "Barrel1 continues in WORK");
    assert_valve_state(VALVES_EXHAUST[0], false, "Barrel0 exhaust valve closed");
    assert_valve_state(VALVES_TO_TURBINE[1], true, "Barrel1 turbine valve still open");
    assert_valve_state(VALVES_INTAKE[0], true, "Barrel0 intake valve opened (preparing)");

    std::cout << "=== Multi-barrel coordination test passed! ===" << std::endl;
}

void test_continuous_energy_production() {
    std::cout << "\n=== Testing continuous energy production (NUM_BARRELS=3) ===" << std::endl;

    // Set up for 3-barrel test to demonstrate continuous energy
    NUM_BARRELS = 3;
    init_valve_states();
    reset_test_state();

    std::cout << "0. System startup: All barrels start in WAIT_FOR_INTAKE..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 chosen to start");
    assert_barrel_state(1, INTAKE, "Barrel1 also starts");
    assert_barrel_state(2, WAIT_FOR_INTAKE, "Barrel2 remains in WAIT_FOR_INTAKE");

    std::cout << "1. Both barrels build pressure..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, INTAKE, "Barrel0 continues building pressure");
    assert_barrel_state(1, INTAKE, "Barrel1 continues building pressure");
    assert_barrel_state(2, WAIT_FOR_INTAKE, "Barrel2 waits for chance to prepare");

    std::cout << "2. Barrel0 reaches pressure first and starts working..." << std::endl;
    mock_pressurized[0] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, WORK, "Barrel0 transitions to WORK");
    assert_barrel_state(1, INTAKE, "Barrel1 continues preparing");
    assert_barrel_state(2, WAIT_FOR_INTAKE, "Barrel2 still waits");

    std::cout << "3. Barrel1 reaches pressure, waits for Barrel0 to finish..." << std::endl;
    mock_pressurized[1] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, WORK, "Barrel0 continues working");
    assert_barrel_state(1, WAIT_FOR_WORK, "Barrel1 transitions to WAIT_FOR_WORK (pressurized, ready)");
    assert_barrel_state(2, INTAKE, "Barrel2 starts preparing (no other barrel in INTAKE)");

    std::cout << "4. Barrel0 water drops - Barrel1 immediately takes over..." << std::endl;
    std::cout << "4. Barrel0 water drops - Barrel1 immediately takes over..." << std::endl;
    mock_water_below[0] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 transitions to EXHAUST");
    assert_barrel_state(1, WORK, "Barrel1 immediately transitions to WORK (was ready)");
    assert_barrel_state(2, INTAKE, "Barrel2 continues preparing");
    assert_valve_state(VALVES_TO_TURBINE[1], true, "Barrel1 turbine valve opened");

    std::cout << "5. Barrel1 working, no gap in energy production..." << std::endl;
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 continues EXHAUST");
    assert_barrel_state(1, WORK, "Barrel1 continues WORK (no gap in energy production)");
    assert_barrel_state(2, INTAKE, "Barrel2 continues preparing");

    std::cout << "6. Barrel2 reaches pressure, waits for Barrel1..." << std::endl;
    mock_pressurized[2] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 continues EXHAUST");
    assert_barrel_state(1, WORK, "Barrel1 continues WORK");
    assert_barrel_state(2, WAIT_FOR_WORK, "Barrel2 transitions to WAIT_FOR_WORK (pressurized, ready)");

    std::cout << "7. Barrel1 water drops - Barrel2 immediately takes over..." << std::endl;
    mock_water_below[1] = true;
    logic();
    print_barrel_states();
    assert_barrel_state(0, EXHAUST, "Barrel0 continues EXHAUST");
    assert_barrel_state(1, EXHAUST, "Barrel1 transitions to EXHAUST");
    assert_barrel_state(2, WORK, "Barrel2 immediately transitions to WORK (was ready)");
    assert_valve_state(VALVES_TO_TURBINE[2], true, "Barrel2 turbine valve opened");

    std::cout << "=== Continuous energy production test passed! ===" << std::endl;
}

void print_barrel_states_with_timing() {
    std::cout << "  Time: " << millis() << "ms - States: ";
    for (int i = 0; i < NUM_BARRELS; i++) {
        std::cout << "Barrel" << i << ":" << state_name(barrel_states[i]);
        if (i < NUM_BARRELS - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

void print_timing_summary() {
    std::cout << "\n=== TIMING SUMMARY ===" << std::endl;
    for (int i = 0; i < NUM_BARRELS; i++) {
        unsigned long avg_intake = get_average_duration(i, INTAKE);
        unsigned long avg_work = get_average_duration(i, WORK);
        unsigned long avg_exhaust = get_average_duration(i, EXHAUST);
        unsigned long avg_wait_intake = get_average_duration(i, WAIT_FOR_INTAKE);
        unsigned long avg_wait_work = get_average_duration(i, WAIT_FOR_WORK);

        if (avg_intake > 0 || avg_work > 0 || avg_exhaust > 0 || avg_wait_intake > 0 || avg_wait_work > 0) {
            std::cout << "Barrel " << i << " averages: ";
            if (avg_intake > 0) std::cout << "INTAKE=" << avg_intake << "ms ";
            if (avg_work > 0) std::cout << "WORK=" << avg_work << "ms ";
            if (avg_exhaust > 0) std::cout << "EXHAUST=" << avg_exhaust << "ms ";
            if (avg_wait_intake > 0) std::cout << "WAIT_INTAKE=" << avg_wait_intake << "ms ";
            if (avg_wait_work > 0) std::cout << "WAIT_WORK=" << avg_wait_work << "ms ";
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}void test_timing_system() {
    std::cout << "\n=== Testing Timing System with Realistic Durations ===" << std::endl;

    NUM_BARRELS = 2;
    init_valve_states();
    init_timing_system();

    // Reset all barrel states to WAIT_FOR_INTAKE
    for (int i = 0; i < MAX_BARRELS; i++) {
        barrel_states[i] = WAIT_FOR_INTAKE;
        mock_pressurized[i] = false;
        mock_water_below[i] = false;
        mock_water_upper[i] = false;
    }

    std::cout << "0. System startup..." << std::endl;
    advance_time(100); // 100ms startup delay
    logic();
    print_barrel_states_with_timing();

    // Simulate INTAKE phase for both barrels (5 seconds)
    std::cout << "1. Both barrels building pressure..." << std::endl;
    for (int cycle = 0; cycle < 50; cycle++) {
        advance_time(100); // 100ms per cycle
        logic();
    }
    print_barrel_states_with_timing();

    // Barrel0 reaches pressure first
    std::cout << "2. Barrel0 reaches pressure first..." << std::endl;
    mock_pressurized[0] = true;
    advance_time(100);
    logic();
    print_barrel_states_with_timing();

    // Barrel1 reaches pressure 2 seconds later
    std::cout << "3. Barrel1 reaches pressure..." << std::endl;
    advance_time(2000);
    mock_pressurized[1] = true;
    logic();
    print_barrel_states_with_timing();

    // Barrel0 works for 3 seconds
    std::cout << "4. Barrel0 working..." << std::endl;
    advance_time(3000);
    mock_water_below[0] = true;
    logic();
    print_barrel_states_with_timing();

    // Barrel0 exhausts for 2 seconds
    std::cout << "5. Barrel0 exhausting..." << std::endl;
    advance_time(2000);
    mock_water_upper[0] = true;
    mock_water_below[0] = false;
    mock_pressurized[0] = false;
    logic();
    print_barrel_states_with_timing();

    // Barrel1 works for 4 seconds
    std::cout << "6. Barrel1 working..." << std::endl;
    advance_time(4000);
    mock_water_below[1] = true;
    logic();
    print_barrel_states_with_timing();

    // Barrel1 exhausts for 2.5 seconds
    std::cout << "7. Barrel1 exhausting..." << std::endl;
    advance_time(2500);
    mock_water_upper[1] = true;
    mock_water_below[1] = false;
    mock_pressurized[1] = false;
    logic();
    print_barrel_states_with_timing();

    print_timing_summary();

    std::cout << "\n=== Predictions based on timing data ===" << std::endl;
    unsigned long barrel0_cycle_time = get_average_duration(0, INTAKE) + get_average_duration(0, WORK) + get_average_duration(0, EXHAUST);
    unsigned long barrel1_cycle_time = get_average_duration(1, INTAKE) + get_average_duration(1, WORK) + get_average_duration(1, EXHAUST);

    if (barrel0_cycle_time > 0) {
        std::cout << "Barrel0 full cycle time: " << barrel0_cycle_time << "ms" << std::endl;
        std::cout << "  -> Should start INTAKE " << (get_average_duration(0, WORK) + get_average_duration(0, EXHAUST))
                  << "ms before current barrel finishes" << std::endl;
    }

    if (barrel1_cycle_time > 0) {
        std::cout << "Barrel1 full cycle time: " << barrel1_cycle_time << "ms" << std::endl;
        std::cout << "  -> Should start INTAKE " << (get_average_duration(1, WORK) + get_average_duration(1, EXHAUST))
                  << "ms before current barrel finishes" << std::endl;
    }

    std::cout << "=== Timing system test passed! ===" << std::endl;
}

int main() {
    test_single_barrel_cycle();
    test_multi_barrel_coordination();
    test_continuous_energy_production();
    test_timing_system();
    return 0;
}
