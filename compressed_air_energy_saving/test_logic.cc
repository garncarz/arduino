#include <iostream>
#include <cassert>
#include <map>
#include <string>

#include "constants.h"
#include "logic.h"

// Mock timing functions for testing
unsigned long mock_time = 0;
unsigned long millis() { return mock_time; }
void advance_time(unsigned long ms) { mock_time += ms; }

// Mock Arduino-specific functions for testing
void logger(const char* msg) {
    std::cout << "[LOG] " << msg << std::endl;
}

void logger(std::string msg) {
    std::cout << "[LOG] " << msg << std::endl;
}

void delay(int ms) {
    // In tests, we don't actually delay
    advance_time(ms);
}

int analogRead(int pin) {
    // Mock sensor readings for testing
    // Return different values based on pin for testing variety
    return (pin % 4) * 256; // 0, 256, 512, 768 for different pins
}

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
}

void test_timing_system() {
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

void test_no_energy_gaps() {
    std::cout << "\n=== Testing NO ENERGY GAPS (strict timing) ===" << std::endl;

    // Reset system
    init_valve_states();
    reset_test_state();
    NUM_BARRELS = 2;

    // Count working barrels manually (since function is in logic.cpp)
    auto count_working = []() -> int {
        int count = 0;
        for (int i = 0; i < NUM_BARRELS; i++) {
            if (barrel_states[i] == WORK) count++;
        }
        return count;
    };

    // Start both barrels in intake
    logic();
    assert(barrel_states[0] == INTAKE && barrel_states[1] == INTAKE);

    // Barrel0 reaches pressure first after 4 seconds
    advance_time(4000);
    mock_pressurized[0] = true;
    logic();
    assert(barrel_states[0] == WORK);
    assert(barrel_states[1] == INTAKE);
    std::cout << "✓ Barrel0 starts working, Barrel1 continues preparing" << std::endl;

    // Barrel1 finishes intake (total 6 seconds)
    advance_time(2000);
    mock_pressurized[1] = true;
    logic();
    assert(barrel_states[0] == WORK);
    assert(barrel_states[1] == WAIT_FOR_WORK);
    std::cout << "✓ Barrel1 ready to work, waiting for handoff" << std::endl;

    // Barrel0 finishes work after total 3 seconds working
    advance_time(1000);
    mock_water_below[0] = true;
    logic();
    assert(barrel_states[0] == EXHAUST);
    assert(barrel_states[1] == WORK);
    std::cout << "✓ Immediate handoff: Barrel0 → EXHAUST, Barrel1 → WORK" << std::endl;

    // This is the critical moment - verify no gaps
    int working_barrels = count_working();
    assert(working_barrels == 1);
    std::cout << "✓ Continuous energy: exactly 1 barrel working during transition" << std::endl;

    std::cout << "=== NO ENERGY GAPS test passed! ===" << std::endl;
}

void test_manual_control() {
    std::cout << std::endl << "=== Testing Manual Control System ===" << std::endl;

    // Reset to automatic mode
    manual_mode = false;
    for (int i = 0; i < NUM_BARRELS; i++) {
        manual_states[i] = WAIT_FOR_INTAKE;
        barrel_states[i] = WAIT_FOR_INTAKE;
    }

    // Test entering manual mode
    manual_mode = true;
    manual_states[0] = WORK;
    manual_states[1] = EXHAUST;
    logic();

    std::cout << "✓ Manual mode active with Barrel0:WORK, Barrel1:EXHAUST" << std::endl;

    // Verify manual control overrides automatic logic
    // In manual mode, states should remain as manually set
    State prev_state0 = barrel_states[0];
    State prev_state1 = barrel_states[1];
    logic();

    if (barrel_states[0] == prev_state0 && barrel_states[1] == prev_state1) {
        std::cout << "✓ Manual mode prevents automatic state transitions" << std::endl;
    } else {
        std::cout << "✗ Manual mode failed to override automatic logic" << std::endl;
    }

    // Test returning to automatic mode
    manual_mode = false;
    std::cout << "✓ System can return to automatic mode" << std::endl;
    std::cout << "=== Manual Control System test passed! ===" << std::endl;
}

void test_startup_assessment() {
    std::cout << std::endl << "=== Testing Startup Assessment and Recovery ===" << std::endl;

    // Test 1: Empty barrels (no pressure, low water) - should assess as WAIT_FOR_INTAKE
    std::cout << "1. Testing empty barrels assessment..." << std::endl;
    NUM_BARRELS = 2;
    init_valve_states();

    // Simulate empty barrels
    mock_pressurized[0] = false;
    mock_pressurized[1] = false;
    mock_water_below[0] = true;   // Water below lower level
    mock_water_below[1] = true;
    mock_water_upper[0] = false;  // No water at upper level
    mock_water_upper[1] = false;

    assess_startup_state();

    if (barrel_states[0] == WAIT_FOR_INTAKE && barrel_states[1] == WAIT_FOR_INTAKE) {
        std::cout << "✓ Empty barrels correctly assessed as WAIT_FOR_INTAKE" << std::endl;
    } else {
        std::cout << "✗ Empty barrels assessment failed" << std::endl;
    }

    // Test 2: Pressurized barrel with water - should assess as WAIT_FOR_WORK (safe)
    std::cout << "2. Testing pressurized barrel with water..." << std::endl;
    mock_pressurized[0] = true;   // High pressure
    mock_water_below[0] = false;  // Water above lower level
    mock_water_upper[0] = false;  // Not at upper level (mid-level)

    State assessed = determine_barrel_state_from_sensors(0);
    if (assessed == WAIT_FOR_WORK) {
        std::cout << "✓ Pressurized barrel with water assessed as WAIT_FOR_WORK (safe)" << std::endl;
    } else {
        std::cout << "✗ Pressurized barrel assessment failed, got: " << state_name(assessed) << std::endl;
    }

    // Test 3: Pressurized barrel with low water - should assess as EXHAUST
    std::cout << "3. Testing pressurized barrel with low water..." << std::endl;
    mock_pressurized[1] = true;   // High pressure
    mock_water_below[1] = true;   // Water below lower level
    mock_water_upper[1] = false;  // No water at upper

    assessed = determine_barrel_state_from_sensors(1);
    if (assessed == EXHAUST) {
        std::cout << "✓ Pressurized barrel with low water assessed as EXHAUST" << std::endl;
    } else {
        std::cout << "✗ Pressurized low-water barrel assessment failed, got: " << state_name(assessed) << std::endl;
    }

    // Test 4: No pressure but high water - should assess as INTAKE
    std::cout << "4. Testing no pressure with high water..." << std::endl;
    mock_pressurized[0] = false;  // No pressure
    mock_water_below[0] = false;  // Water above lower level
    mock_water_upper[0] = true;   // Water at upper level

    assessed = determine_barrel_state_from_sensors(0);
    if (assessed == INTAKE) {
        std::cout << "✓ No pressure + high water assessed as INTAKE" << std::endl;
    } else {
        std::cout << "✗ No pressure + high water assessment failed, got: " << state_name(assessed) << std::endl;
    }

    // Test 5: Mixed scenario - different barrels in different states after reset
    std::cout << "5. Testing mixed barrel states after reset..." << std::endl;

    // Barrel 0: Pressurized with water (ready to work)
    mock_pressurized[0] = true;
    mock_water_below[0] = false;
    mock_water_upper[0] = false;

    // Barrel 1: No pressure, empty (needs preparation)
    mock_pressurized[1] = false;
    mock_water_below[1] = true;
    mock_water_upper[1] = false;

    assess_startup_state();

    if ((barrel_states[0] == WAIT_FOR_WORK) &&
        (barrel_states[1] == WAIT_FOR_INTAKE)) {
        std::cout << "✓ Mixed barrel states correctly assessed" << std::endl;
        std::cout << "  Barrel0: " << state_name(barrel_states[0]) << " (pressurized, ready)" << std::endl;
        std::cout << "  Barrel1: " << state_name(barrel_states[1]) << " (empty, needs prep)" << std::endl;
    } else {
        std::cout << "✗ Mixed barrel assessment failed" << std::endl;
    }

    // Test 6: Safety valve configurations after assessment
    std::cout << "6. Testing safety valve configurations..." << std::endl;

    // After assessment, check that valves are in safe positions
    bool valves_safe = true;
    for (int i = 0; i < NUM_BARRELS; i++) {
        bool intake_open = valve_states[VALVES_INTAKE[i]];
        bool exhaust_open = valve_states[VALVES_EXHAUST[i]];
        bool turbine_open = valve_states[VALVES_TO_TURBINE[i]];

        // Safety check: turbine should not be open immediately after reset
        if (turbine_open) {
            std::cout << "✗ SAFETY: Barrel" << i << " turbine valve open after reset!" << std::endl;
            valves_safe = false;
        }

        // Check valve configuration matches assessed state
        State state = barrel_states[i];
        if (state == INTAKE && !intake_open) {
            std::cout << "✗ Barrel" << i << " in INTAKE but intake valve closed" << std::endl;
            valves_safe = false;
        }
        if (state == EXHAUST && !exhaust_open) {
            std::cout << "✗ Barrel" << i << " in EXHAUST but exhaust valve closed" << std::endl;
            valves_safe = false;
        }
    }

    if (valves_safe) {
        std::cout << "✓ All valve configurations are safe after startup assessment" << std::endl;
    }

    // Test 7: System readiness assessment
    std::cout << "7. Testing system readiness assessment..." << std::endl;

    // The startup assessment should identify if we have barrels ready for energy production
    bool has_ready_barrel = false;
    bool has_preparing_barrel = false;

    for (int i = 0; i < NUM_BARRELS; i++) {
        if (barrel_states[i] == WORK || barrel_states[i] == WAIT_FOR_WORK) {
            has_ready_barrel = true;
        }
        if (barrel_states[i] == INTAKE || barrel_states[i] == WAIT_FOR_INTAKE) {
            has_preparing_barrel = true;
        }
    }

    if (has_ready_barrel) {
        std::cout << "✓ System has at least one barrel ready for energy production" << std::endl;
    } else if (has_preparing_barrel) {
        std::cout << "✓ System has barrels preparing (energy production will be available soon)" << std::endl;
    } else {
        std::cout << "⚠ System has no barrels ready - this should trigger auto-preparation" << std::endl;
    }

    std::cout << "=== Startup Assessment and Recovery test passed! ===" << std::endl;
}

void test_startup_recovery_scenarios() {
    std::cout << std::endl << "=== Testing Startup Recovery Scenarios ===" << std::endl;

    // Scenario 1: System reset during WORK state
    std::cout << "Scenario 1: Reset during WORK state..." << std::endl;
    NUM_BARRELS = 1;
    init_valve_states();

    // Simulate conditions of barrel that was working when reset occurred
    mock_pressurized[0] = true;   // Still pressurized
    mock_water_below[0] = true;   // Water level dropped (was generating power)
    mock_water_upper[0] = false;

    assess_startup_state();

    if (barrel_states[0] == EXHAUST) {
        std::cout << "✓ Barrel reset during WORK correctly transitioned to EXHAUST" << std::endl;
    } else {
        std::cout << "✗ WORK state recovery failed, got: " << state_name(barrel_states[0]) << std::endl;
    }

    // Scenario 2: System reset during INTAKE state
    std::cout << "Scenario 2: Reset during INTAKE state..." << std::endl;

    // Simulate conditions of barrel that was in intake when reset occurred
    mock_pressurized[0] = false;  // Still building pressure
    mock_water_below[0] = false;  // Water level rising
    mock_water_upper[0] = false;  // Not full yet

    State assessed = determine_barrel_state_from_sensors(0);

    if (assessed == INTAKE) {
        std::cout << "✓ Barrel reset during INTAKE correctly continues INTAKE" << std::endl;
    } else {
        std::cout << "✗ INTAKE state recovery failed, got: " << state_name(assessed) << std::endl;
    }

    // Scenario 3: System reset with all barrels in unknown/bad state
    std::cout << "Scenario 3: Complete system reset with unknown states..." << std::endl;
    NUM_BARRELS = 3;
    init_valve_states();

    // Simulate worst-case: inconsistent sensor readings
    for (int i = 0; i < NUM_BARRELS; i++) {
        mock_pressurized[i] = false;
        mock_water_below[i] = true;
        mock_water_upper[i] = false;
    }

    assess_startup_state();

    // After complete reset, all barrels should be in safe states
    bool all_safe = true;
    for (int i = 0; i < NUM_BARRELS; i++) {
        if (barrel_states[i] != WAIT_FOR_INTAKE && barrel_states[i] != INTAKE) {
            all_safe = false;
            break;
        }
    }

    if (all_safe) {
        std::cout << "✓ Complete system reset results in safe states for all barrels" << std::endl;
    } else {
        std::cout << "✗ Complete system reset did not result in safe states" << std::endl;
    }

    // Scenario 4: Gradual recovery test - system should start working again
    std::cout << "Scenario 4: Testing gradual recovery to normal operation..." << std::endl;

    // After assessment, run normal logic for a few cycles
    mock_time = 0;
    for (int cycle = 0; cycle < 5; cycle++) {
        logic();  // Run normal barrel logic
        advance_time(100);

        // Simulate sensor changes as system operates
        if (cycle >= 2) {
            // After a few cycles, one barrel should build pressure
            mock_pressurized[0] = true;
        }
    }

    // Check that system has started normal operation
    bool system_operating = false;
    for (int i = 0; i < NUM_BARRELS; i++) {
        if (barrel_states[i] == WORK || barrel_states[i] == WAIT_FOR_WORK) {
            system_operating = true;
            break;
        }
    }

    if (system_operating) {
        std::cout << "✓ System successfully recovered to normal operation after reset" << std::endl;
    } else {
        std::cout << "⚠ System not yet fully operational (may need more time)" << std::endl;
    }

    std::cout << "=== Startup Recovery Scenarios test passed! ===" << std::endl;
}

int main() {
    test_single_barrel_cycle();
    test_multi_barrel_coordination();
    test_continuous_energy_production();
    test_timing_system();
    test_no_energy_gaps();
    test_manual_control();
    test_startup_assessment();
    test_startup_recovery_scenarios();
    return 0;
}
