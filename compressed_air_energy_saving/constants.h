#pragma once

const int VALVE_INTAKE = 2;
const int VALVE_EXHAUST = 3;
const int VALVE_TO_TURBINE = 4;

const int SENSOR_LOWER = 5;
const int SENSOR_UPPER = 6;

// SENSOR_PRESSURE has to be defined in the .ino file because it uses A0 (Arduino-specific)

// Thresholds
const int PRESSURE_TARGET = 700;
