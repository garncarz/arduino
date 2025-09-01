# Compressed Air Energy Saving System

An Arduino-based control system for a multi-barrel compressed air energy storage and recovery system. This project implements intelligent coordination between multiple air compression barrels to achieve continuous energy production while maximizing efficiency.

## 🎯 Overview

This system is based on [Czech Patent CZ 310138](https://isdv.upv.gov.cz/doc/FullFiles/Patents/FullDocuments/310/310138.pdf) and implements a sophisticated state machine for coordinating multiple compressed air barrels to:

- **Continuous Energy Production**: Ensure uninterrupted power generation through intelligent barrel coordination
- **Efficiency Optimization**: Minimize energy waste during barrel transitions
- **Performance Monitoring**: Real-time timing analysis and predictive optimization
- **Scalable Architecture**: Support for 1-4 barrels with configurable coordination logic
- **System Robustness**: Automatic startup assessment and recovery from unexpected resets

## 🏗️ System Architecture

### Multi-Barrel State Machine

Each barrel operates through a 5-state cycle:

```
WAIT_FOR_INTAKE → INTAKE → WORK → EXHAUST → WAIT_FOR_INTAKE
                     ↘    ↗
                  WAIT_FOR_WORK
```

**State Descriptions:**
- **INTAKE**: Pressurizing the barrel with compressed air
- **WORK**: Generating energy by releasing air through turbine
- **EXHAUST**: Releasing remaining pressure and refilling with water
- **WAIT_FOR_INTAKE**: Waiting for opportunity to start preparation
- **WAIT_FOR_WORK**: Ready to work, waiting for current working barrel to finish

### Coordination Logic

The system ensures continuous energy production by:
1. **Overlapping Preparation**: Next barrel starts INTAKE while current barrel is in WORK
2. **Immediate Handoff**: Zero-gap transitions between working barrels
3. **Smart Scheduling**: Prevents resource conflicts during preparation phases
4. **Adaptive Timing**: Uses historical data to optimize preparation timing
5. **2-Barrel Optimization**: Direct EXHAUST→INTAKE transitions eliminate energy gaps
6. **Robust Startup**: Automatic assessment and recovery from unexpected system resets

#### Optimized 2-Barrel Operation
For systems with exactly 2 barrels, the coordination logic has been optimized to eliminate unnecessary WAIT_FOR_INTAKE states:

- **Direct Transitions**: Barrels go directly from EXHAUST to INTAKE
- **Continuous Production**: Always maintains exactly one barrel working
- **Zero Waiting**: WAIT_FOR_INTAKE state bypassed entirely during normal operation
- **Energy Gap Elimination**: Zero downtime between barrel handoffs

This optimization ensures that 2-barrel systems achieve perfect continuous energy production without the coordination overhead needed for 3+ barrel systems.

## 🛡️ System Robustness & Recovery

### Startup Assessment System

The system includes comprehensive startup assessment capabilities to handle unexpected Arduino resets or power interruptions gracefully. When the system starts, it automatically:

1. **Sensor-Based State Assessment**: Analyzes pressure and water level sensors to determine the actual physical state of each barrel
2. **Safe State Recovery**: Transitions barrels to appropriate operational states based on sensor readings
3. **Conservative Safety**: Prioritizes safety over efficiency during recovery (e.g., WORK → WAIT_FOR_WORK transitions)
4. **Valve Safety Protocol**: Ensures all valves are closed initially before making any state transitions

### Recovery Decision Matrix

The startup assessment uses sensor readings to intelligently determine barrel states:

| Pressure | Water Level | Assessed State | Recovery Action |
|----------|-------------|----------------|-----------------|
| High | Above Lower | WAIT_FOR_WORK | All valves closed, ready for coordination |
| High | Below Lower | EXHAUST | Open exhaust valve, complete pressure release |
| Low | At Upper | INTAKE | Continue intake process |
| Low | Below Lower | WAIT_FOR_INTAKE | All valves closed, ready for next cycle |
| Low | Mid-Level | INTAKE | Continue intake process |

### Safety Features

#### Startup Protocol
```cpp
assess_startup_state();  // Called automatically in setup()
```

- **Valve Reset**: All valves closed initially for safety
- **Sensor Stabilization**: Brief delays allow sensors to provide accurate readings
- **Conservative Assessment**: When in doubt, chooses safer operational states
- **Detailed Logging**: Complete assessment results logged for debugging

#### Recovery Scenarios
The system handles various reset scenarios:

- **Reset During INTAKE**: Continues intake process based on current pressure/water levels
- **Reset During WORK**: Transitions to WAIT_FOR_WORK for safe turbine coordination
- **Reset During EXHAUST**: Continues exhaust process to complete pressure release
- **Reset Between States**: Places barrels in appropriate waiting states for coordination

#### Platform Compatibility
The startup assessment system is designed to work across different Arduino platforms:

- **Arduino Uno R3**: Memory-optimized with reduced timing history (3 entries)
- **Arduino Uno R4 WiFi**: Full feature set with extended timing history (10 entries)
- **Conditional Compilation**: Automatically adapts to available memory constraints

### Example Startup Assessment Log
```
=== STARTUP ASSESSMENT ===
System reset detected - assessing barrel states from sensors...
Barrel0 sensors: P=385 (HIGH) U=NO_WATER L=WATER
Barrel0 assessed as: WAIT_FOR_WORK
Barrel0 recovery: All valves closed, waiting
Barrel1 sensors: P=145 (LOW) U=WATER L=WATER
Barrel1 assessed as: INTAKE
Barrel1 recovery: Intake valve opened
Startup assessment complete: Barrel0:WAIT_FOR_WORK | Barrel1:INTAKE
=== STARTUP ASSESSMENT COMPLETE ===
```

## 🔧 Hardware Components

![Circuit Diagram](schema.jpg)

### Per Barrel (up to 4 barrels supported):
- **3 Solenoid Valves** (controlled via Songle SRD-5VDC-SL-C relay module):
  - Intake valve (air compression)
  - Exhaust valve (pressure release)
  - Turbine valve (energy generation)
- **2 Water Level Sensors**:
  - Lower level sensor (work completion detection)
  - Upper level sensor (exhaust completion detection)
- **1 Pressure Sensor**: Monitors compression level

**Note**: The system uses low-triggered relay modules (active LOW) - valves open when Arduino pin is LOW, close when HIGH.

### Arduino Connections

| Component | Barrel 0 | Barrel 1 | Barrel 2 | Barrel 3 |
|-----------|----------|----------|----------|----------|
| Intake Valve | Pin 2 | Pin 7 | Pin 12 | Pin 22 |
| Exhaust Valve | Pin 3 | Pin 8 | Pin 13 | Pin 23 |
| Turbine Valve | Pin 4 | Pin 9 | Pin 14 | Pin 24 |
| Lower Sensor | Pin 5 | Pin 10 | Pin 15 | Pin 25 |
| Upper Sensor | Pin 6 | Pin 11 | Pin 16 | Pin 26 |
| Pressure Sensor | A0 | A1 | A2 | A3 |

## 📊 Performance Monitoring

### Real-Time Timing Analysis
The system continuously monitors and records:
- **State Durations**: Time spent in each operational state
- **Wait Times**: Efficiency metrics for coordination delays
- **Cycle Predictions**: Optimal timing for barrel preparation
- **Performance Trends**: Historical analysis for system optimization

### WiFi Logging (Arduino Uno R4 WiFi)
For remote monitoring, the system supports WiFi-based UDP logging:

**Setup:**
1. Create `wifi_credentials.h` file with your network details:
   ```cpp
   #ifndef WIFI_CREDENTIALS_H
   #define WIFI_CREDENTIALS_H
   const char* WIFI_SSID = "YourWiFiName";
   const char* WIFI_PASSWORD = "YourPassword";
   #endif
   ```

2. The system broadcasts log messages to `255.255.255.255:1768` (UDP)

**Monitoring Options:**
- **Android**: [UDP Terminal](https://play.google.com/store/apps/details?id=com.hardcodedjoy.udpterminal) - Listen on port 1768
- **Computer**: `nc -u -l 1768` (netcat command)
- **Phone Hotspot**: Works perfectly with Android hotspot networks

**Benefits:**
- Real-time monitoring without serial cable connection
- Remote system diagnostics and performance analysis
- Continuous logging during field operations
- Multiple devices can receive the same log stream simultaneously

### Serial Output Example
```
Compressed Air Energy System Started
Barrel0:INTAKE (P:145 U:0 L:0) | Barrel1:INTAKE (P:135 U:0 L:0)
Barrel0:INTAKE (P:205 U:0 L:0) | Barrel1:INTAKE (P:195 U:0 L:0)
Barrel0:INTAKE (P:265 U:0 L:0) | Barrel1:INTAKE (P:255 U:0 L:0)
Barrel0:INTAKE (P:325 U:0 L:0) | Barrel1:INTAKE (P:315 U:0 L:0)
Barrel0:WORK (P:385 U:0 L:0) | Barrel1:INTAKE (P:375 U:0 L:0)
Barrel0:WORK (P:345 U:0 L:0) | Barrel1:WAIT_FOR_WORK (P:375 U:0 L:0)
Barrel0:WORK (P:305 U:0 L:0) | Barrel1:WAIT_FOR_WORK (P:375 U:0 L:0)
Barrel0:WORK (P:265 U:0 L:1) | Barrel1:WAIT_FOR_WORK (P:375 U:0 L:0)
Barrel0:EXHAUST (P:205 U:1 L:1) | Barrel1:WORK (P:335 U:0 L:0)
Barrel0:EXHAUST (P:145 U:1 L:1) | Barrel1:WORK (P:295 U:0 L:0)
Barrel0:INTAKE (P:125 U:0 L:0) | Barrel1:WORK (P:255 U:0 L:1)
Barrel0:INTAKE (P:185 U:0 L:0) | Barrel1:WORK (P:215 U:0 L:1)
Barrel0:WAIT_FOR_WORK (P:385 U:0 L:0) | Barrel1:WORK (P:175 U:0 L:1)
Barrel0:WORK (P:385 U:0 L:0) | Barrel1:EXHAUST (P:135 U:1 L:1)
Barrel0:WORK (P:345 U:0 L:0) | Barrel1:INTAKE (P:95 U:0 L:0)

=== TIMING SUMMARY ===
Barrel 0 averages: INTAKE=5100ms WORK=5000ms EXHAUST=2000ms
Barrel 1 averages: INTAKE=7100ms WORK=6000ms EXHAUST=2500ms

=== TIMING PREDICTIONS ===
Barrel0 cycle: 12100ms, start INTAKE 7000ms early
Barrel1 cycle: 14600ms, start INTAKE 8000ms early
```

**Key Optimization**: Notice how in the optimized 2-barrel system, Barrel0 goes directly from EXHAUST to INTAKE (line 11) and Barrel1 does the same (line 15), eliminating WAIT_FOR_INTAKE states that could cause energy gaps.

## 🎛️ Manual Control Mode

The system supports manual override of automatic barrel coordination for testing, maintenance, and custom operations.

### Command Interface
Manual commands can be sent via:
- **Serial connection** (9600 baud)
- **WiFi UDP** (port 1768, same as logging port)

### Available Commands

#### Mode Control
```
MODE AUTO    # Switch to automatic operation
MODE MANUAL  # Enable manual control mode
```

#### Direct Barrel Control
```
CMD <STATE> <BARREL>    # Set specific barrel to specific state
```

**Examples:**
```
CMD INTAKE 0      # Force Barrel 0 to INTAKE state
CMD WORK 1        # Force Barrel 1 to WORK state
CMD EXHAUST 0     # Force Barrel 0 to EXHAUST state
```

**Supported States:**
- `INTAKE` - Pressurize barrel with compressed air
- `WORK` - Generate energy through turbine
- `EXHAUST` - Release pressure and refill with water

#### System Status
```
STATUS    # Display current mode and all barrel states
HELP      # Show available commands
```

### Manual Mode Features

#### Global Control
- **Single Mode Setting**: One global manual mode affects all barrels
- **Persistent State**: Manual mode remains active until explicitly disabled
- **No Timeouts**: Manual control persists indefinitely (no automatic return to auto mode)
- **Override Protection**: Manual mode completely overrides automatic state machine

#### Safety & Flexibility
- **Real-time Switching**: Can switch between manual and automatic at any time
- **State Preservation**: Manual states persist until explicitly changed
- **Full Valve Control**: Manual mode directly controls valve positions
- **Monitoring Continues**: Sensor readings and logging continue in manual mode

### Example Manual Session
```bash
# Connect via serial or UDP
> MODE MANUAL
Manual mode activated

> STATUS
Mode: MANUAL
Barrel0: EXHAUST  Barrel1: WORK

> CMD INTAKE 0
Barrel 0 set to INTAKE

> CMD EXHAUST 1
Barrel 1 set to EXHAUST

> STATUS
Mode: MANUAL
Barrel0: INTAKE  Barrel1: EXHAUST

> MODE AUTO
Automatic mode activated
```

### WiFi Manual Control
For remote operation, you can send commands via UDP to the same port used for logging:

**Using netcat (Linux/Mac):**
```bash
echo "MODE MANUAL" | nc -u -w1 192.168.1.100 1768
echo "CMD WORK 0" | nc -u -w1 192.168.1.100 1768
echo "STATUS" | nc -u -w1 192.168.1.100 1768
```

**Using UDP Terminal (Android):**
1. Set target IP to Arduino's WiFi IP
2. Set port to 1768
3. Type commands and press send

### Use Cases
- **System Testing**: Manually cycle through barrel states for validation
- **Maintenance**: Isolate specific barrels for inspection
- **Troubleshooting**: Force specific states to diagnose issues
- **Custom Operations**: Non-standard barrel coordination patterns
- **Emergency Control**: Override automatic logic in fault conditions

## 🚀 Getting Started

### Prerequisites
- Arduino IDE or compatible development environment
- Arduino Uno/Nano or compatible board
- Hardware components as listed above

### Installation
1. Clone this repository
2. Open `compressed_air_energy_saving.ino` in Arduino IDE
3. Configure `NUM_BARRELS` in the code for your setup (1-4)
4. Upload to your Arduino board

### Configuration
Edit `constants.h` to match your hardware setup:
```cpp
// Number of barrels in your system
int NUM_BARRELS = 2;  // Change to 1, 2, 3, or 4

// Adjust pressure threshold if needed
const int PRESSURE_TARGET = 700;  // Analog reading threshold
```

## 🧪 Testing

### Run Unit Tests
```bash
make test
```

This now runs comprehensive testing including:
- **Logic Tests**: C++ unit tests for barrel coordination and state machine
- **Arduino R3 Compatibility**: Compilation test for Arduino Uno with reduced features
- **Arduino R4 WiFi Compatibility**: Full system compilation test for R4 WiFi

Individual test components can be run separately:
```bash
make test-logic          # Run only logic/unit tests
make test-arduino-r3     # Test Arduino R3 compatibility
make test-arduino-r4     # Test Arduino R4 WiFi compatibility
make setup               # Install Arduino CLI and setup environment
```

### Test Coverage
- Single barrel operation cycle
- Multi-barrel coordination logic
- Continuous energy production verification
- **2-barrel optimization validation**: Zero energy gaps testing
- **WAIT_FOR_INTAKE efficiency**: Minimization of unnecessary wait states
- **Startup assessment testing**: Sensor-based state recovery validation
- **Recovery scenario testing**: Various reset conditions and appropriate responses
- Timing system validation
- State transition correctness

### CI/CD
Automated testing via GitHub Actions:
- Validates logic correctness
- Confirms Arduino compilation compatibility (both Uno R3 and R4 WiFi)
- Memory optimization validation for different Arduino platforms
- Ensures code quality and reliability

## 🤖 Development Methodology

This project's code and comprehensive test suite were primarily written by GitHub Copilot (Claude Sonnet 4) through iterative prompting. The logic is validated both through rigorous software testing and real Arduino hardware verification.

## 📁 Project Structure

```
compressed_air_energy_saving/
├── compressed_air_energy_saving.ino  # Main Arduino sketch
├── logic.cpp                         # Core state machine logic
├── logic.h                           # Logic interface and definitions
├── constants.h                       # Hardware pin mappings and constants
├── test_logic.cc                     # Comprehensive test suite
├── Makefile                          # Build and test automation
├── schema.fzz/.jpg                   # Fritzing circuit diagrams
└── README.md                         # This file
```

## 🔬 Technical Details

### State Machine Implementation
- **Arduino-optimized**: Uses simple arrays instead of complex data structures
- **Memory efficient**: Suitable for Arduino Uno's 2KB SRAM
- **Timing precision**: Millisecond-accurate state duration tracking
- **Robust coordination**: Prevents conflicts and ensures system stability

### Key Algorithms
- **Barrel Selection**: Intelligent choosing of next barrel to prepare
- **Transition Timing**: Optimal handoff timing based on historical data
- **Resource Management**: Prevents simultaneous access to shared resources
- **Performance Optimization**: Continuous improvement through timing analysis
- **2-Barrel Coordination**: Specialized logic for optimal 2-barrel continuous operation
- **Startup Assessment**: Sensor-based state recovery for robust system restarts

## 🎛️ Configuration Options

### Single Barrel Mode (`NUM_BARRELS = 1`)
- Simple cycle: INTAKE → WORK → EXHAUST → repeat
- Ideal for testing and basic energy storage

### Two-Barrel Mode (`NUM_BARRELS = 2`) - **Optimized**
- **Continuous energy production** with zero gaps
- **Direct transitions**: EXHAUST → INTAKE (bypasses WAIT_FOR_INTAKE)
- **Perfect coordination**: Always exactly one barrel working
- **Zero overhead**: Eliminates waiting states entirely

### Multi-Barrel Mode (`NUM_BARRELS = 3-4`)
- Advanced continuous energy production
- Overlapping barrel preparation with smart scheduling
- WAIT_FOR_INTAKE coordination prevents resource conflicts
- Zero-gap energy handoffs with predictive timing

## 📈 Performance Optimization

The system provides real-time insights for optimization:
- **Cycle Time Analysis**: Identify bottlenecks in barrel operations
- **Wait Time Monitoring**: Measure coordination efficiency
- **Predictive Timing**: Optimize preparation start times
- **Historical Trends**: Long-term performance analysis
- **2-Barrel Efficiency**: Specialized optimization eliminating energy gaps
- **State Minimization**: Reduced WAIT_FOR_INTAKE usage for improved performance

## 🔗 References

- **Patent Basis**: [CZ 310138 - Compressed Air Energy Storage System](https://isdv.upv.gov.cz/doc/FullFiles/Patents/FullDocuments/310/310138.pdf)
- **Circuit Diagrams**: `schema.fzz` (Fritzing format)

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Run tests: `make test`
4. Ensure Arduino compatibility
5. Submit a pull request

## 📄 License

This project implements concepts from Czech Patent CZ 310138. Please review patent documentation for usage rights and restrictions.

---

**🎯 Project Goal**: Achieve continuous, efficient compressed air energy production through intelligent multi-barrel coordination and real-time performance optimization.
