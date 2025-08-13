#pragma once

enum State { INTAKE, WORK, EXHAUST };
extern State current_state;

bool pressurized_enough();
bool water_below_lower_level();
bool water_reached_upper_level();
void open_valve(int);
void close_valve(int);

void logic();
