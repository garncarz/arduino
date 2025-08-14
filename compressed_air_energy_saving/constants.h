#pragma once

// Maximum number of barrels supported (for array sizing)
const int MAX_BARRELS = 4;

// Number of barrels in the system (can be changed to 1, 2, 3, etc.)
extern int NUM_BARRELS; // Will be defined in the implementation files

// Valve pin arrays - each barrel has 3 valves (sized for maximum)
const int VALVES_INTAKE[MAX_BARRELS] = {2, 7, 12, 22};
const int VALVES_EXHAUST[MAX_BARRELS] = {3, 8, 13, 23};
const int VALVES_TO_TURBINE[MAX_BARRELS] = {4, 9, 14, 24};

// Sensor pin arrays - each barrel has 2 water level sensors (sized for maximum)
const int SENSORS_LOWER[MAX_BARRELS] = {5, 10, 15, 25};
const int SENSORS_UPPER[MAX_BARRELS] = {6, 11, 16, 26};

// Pressure sensor pins - each barrel has its own pressure sensor
// Note: These are analog pins, so use A0, A1, A2, etc.
extern const int SENSORS_PRESSURE[MAX_BARRELS]; // Defined in .ino file

// Thresholds
const int PRESSURE_TARGET = 700;
