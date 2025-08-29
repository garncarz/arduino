#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>

// Forward declaration for log function
void log(const String& message);

// Command processing function
void process_command(String command);

#endif // COMMANDS_H
