# Arduino Compressed Air Energy Saving - Android Controller

Android application for monitoring and controlling the compressed air energy saving system running on Arduino Uno R4 WiFi.

## Overview

This Android app provides real-time monitoring and control of a multi-barrel compressed air energy production system. The system uses water-filled barrels to store compressed air energy and automatically manages the energy production cycle to ensure continuous power output.

## ✨ Key Features

### Real-Time Monitoring
- **Live barrel status**: Display current state of each barrel (INTAKE, WORK, EXHAUST, WAIT_FOR_WORK, WAIT_FOR_INTAKE)
- **Sensor data**: Show pressure readings and water level sensors (upper/lower) for each barrel
- **Continuous monitoring**: Parse and display sensor values in both AUTO and MANUAL modes
- **System mode**: Shows whether system is in AUTO or MANUAL mode
- **Live logs**: Stream of Arduino log messages via UDP broadcast
- **Network info**: Display Arduino IP address and connection status
- **Manual mode visibility**: Even when in manual control, all sensor readings remain visible

### Smart Control Interface
- **Mode switching**: Switch between AUTO and MANUAL operation modes with preserved states
- **Visual feedback**: **Send Barrel Command button changes color to match target barrel state**
  - 🔴 **INTAKE**: Red (dangerous - pressure building)
  - 🟠 **WORK**: Orange (active operation)
  - 🔵 **EXHAUST**: Blue (safe release)
  - 🟢 **WAIT_FOR_INTAKE**: Green (safe waiting)
  - 🟣 **WAIT_FOR_WORK**: Purple (pressurized and ready)
- **State preservation**: **Manual mode preserves current barrel states** (no reset to old manual states)
- **Manual barrel control**: Directly set individual barrel states when in manual mode
- **Automatic mode switching**: CMD commands automatically enable manual mode for convenience
- **System status**: Request current system status on demand

## 🔧 Technical Details

### Communication Protocol
- **Protocol**: UDP on port 1768
- **Arduino → Android**: Broadcast logs to subnet broadcast address (e.g., 192.168.1.255)
- **Android → Arduino**: Send commands to specific Arduino IP or subnet broadcast
- **Command format**:
  - `MODE AUTO` - Switch to automatic mode
  - `MODE MANUAL` - Switch to manual mode
  - `CMD BARREL<n> <STATE>` - Set barrel state (auto-switches to manual)
  - `STATUS` - Request system status
  - `HELP` - Show available commands

### Architecture
- **MVVM Pattern**: Clean separation between UI, business logic, and data layers
- **Service-based**: Background UDP service maintains persistent connection with Arduino
- **LiveData**: Reactive UI updates when system state changes
- **Centralized Colors**: `BarrelStateColors` utility for consistent visual feedback
- **Automatic Status Requests**: App automatically requests status when connection is established
- **Dual Mode Detection**: Handles both command responses and status query responses

### Network Requirements
- Android device and Arduino must be on the same WiFi network
- Arduino broadcasts to subnet broadcast address (more reliable than 255.255.255.255)
- **Required Permissions**:
  - `INTERNET` - Required for UDP socket access
  - `ACCESS_NETWORK_STATE` - Read network connection state
  - `ACCESS_WIFI_STATE` - Read WiFi connection details and network info
  - `ACCESS_COARSE_LOCATION` + `ACCESS_FINE_LOCATION` - Required for WiFi network discovery on Android 6+ (API 23+)
  - `CHANGE_WIFI_STATE` - Optional: If app needs to connect to different networks

### Arduino System States
- **WAIT_FOR_INTAKE**: Barrel waiting for opportunity to start intake cycle
- **INTAKE**: Barrel filling with water and building air pressure
- **WAIT_FOR_WORK**: Barrel pressurized and ready to work, waiting for handoff
- **WORK**: Barrel actively generating power through turbine
- **EXHAUST**: Barrel releasing pressure and emptying water

### System Logic
- **Automatic Mode**: System manages barrel transitions to ensure continuous energy production
- **Manual Mode**: User has direct control over individual barrel states
- **No Energy Gaps**: Multi-barrel coordination ensures always exactly one barrel is working
- **Optimized Handoffs**: 2+ barrel systems eliminate WAIT_FOR_INTAKE states for continuous operation

## 📱 Installation & Setup

### Prerequisites
- Android 5.0+ (API level 21+)
- Arduino Uno R4 WiFi with compressed air energy saving firmware
- Both devices on the same WiFi network

### Building from Source
1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd compressed_air_energy_saving/android
   ```

2. **Build the APK**:
   ```bash
   ./gradlew assembleDebug
   ```

3. **Install on device**:
   ```bash
   # Via USB debugging (device must be authorized)
   adb install ./app/build/outputs/apk/debug/app-debug.apk
   
   # Or transfer APK to device and install manually
   ```

4. **Grant permissions**: App will request network permissions on first launch

### Usage
1. **Connection**: App automatically discovers Arduino on the network
2. **Monitoring**: View real-time barrel states, pressure readings, and sensor data
3. **Mode Control**: Switch between AUTO and MANUAL modes using the toggle buttons
4. **Manual Control**: In MANUAL mode, use color-coded barrel command buttons to control individual barrels
5. **Visual Feedback**: Button colors indicate the target state for easy identification
6. **Status Updates**: Use STATUS button to refresh system information

## 🧪 Testing

The app includes a comprehensive test suite with **32 test methods** across **4 test classes**:

### Running Tests
```bash
# Run all tests with summary
./run_tests.sh

# Or run tests directly with Gradle
./gradlew test
```

### Test Coverage
- **ArduinoDataParserTest** (15 tests): Message parsing, mode detection, barrel data extraction
- **UdpCommunicationServiceTest** (5 tests): Automatic status requests and state management  
- **ModeDetectionIntegrationTest** (8 tests): End-to-end scenarios including app restart and mode synchronization
- **BarrelStateColorsTest** (4 tests): Color utility consistency for visual feedback

### Key Test Scenarios
- Mode detection after app restart (addresses issue where app showed AUTO when Arduino was in MANUAL)
- Automatic STATUS request when connection is established
- Dual format mode detection: "System set to X mode" and "Mode: X" responses
- Barrel state parsing with sensor data preservation during mode changes
- Color mapping consistency for visual feedback
- Robust error handling and edge cases

## 🏆 Recent Improvements

### ✅ Issues Fixed
1. **README Header Corruption**: Recreated clean README with proper structure
2. **Send Barrel Command Button Color Coding**: Button now changes color to match selected barrel state
3. **Mode Switching State Preservation**: Manual mode now preserves current barrel states instead of reverting to old manual states

### ✅ New Features Added
1. **Centralized Color Management**: `BarrelStateColors` utility class for consistent UI
2. **Enhanced Visual Feedback**: Dynamic button coloring with safety-oriented color scheme
3. **Comprehensive Test Suite**: 32 test methods ensuring reliability
4. **Improved Documentation**: Clean structure with usage instructions and architecture details

### ✅ Code Quality Improvements
1. **Better Architecture**: Centralized utilities, clean separation of concerns
2. **Enhanced Error Handling**: Try-catch blocks with fallback behaviors
3. **Testing Infrastructure**: Unit and integration tests with convenient test runner

## 🚀 Future Enhancements

- **Data visualization**: Charts showing energy production over time
- **Scheduling**: Automated mode switching based on time/conditions
- **Multiple Arduino support**: Manage multiple systems from one app
- **Export logs**: Save system logs for analysis
- **Push notifications**: Alerts for system state changes or errors
- **Network discovery**: Automatically find Arduino devices on network

## 📁 Related Files

- **Arduino Code**: `../compressed_air_energy_saving.ino` - Main Arduino sketch
- **WiFi Module**: `../wifi.cpp` - Arduino WiFi communication handling
- **Command Processing**: `../commands.cpp` - Arduino command parsing
- **System Logic**: `../logic.cpp` - Core barrel state management
- **Documentation**: `../README.md` - Arduino system documentation

## 🎯 System Architecture

```
┌─────────────────┐    UDP     ┌──────────────────┐
│  Android App    │ ◄────────► │ Arduino R4 WiFi  │
│                 │   :1768    │                  │
│ ┌─────────────┐ │            │ ┌──────────────┐ │
│ │ UDP Service │ │            │ │ WiFi Module  │ │
│ └─────────────┘ │            │ └──────────────┘ │
│ ┌─────────────┐ │            │ ┌──────────────┐ │
│ │ State Parse │ │            │ │ Logic Engine │ │
│ └─────────────┘ │            │ └──────────────┘ │
│ ┌─────────────┐ │            │ ┌──────────────┐ │
│ │ UI Activity │ │            │ │ Barrel Ctrl  │ │
│ └─────────────┘ │            │ └──────────────┘ │
└─────────────────┘            └──────────────────┘
```

---

**Status**: ✅ Production Ready - Comprehensive testing, clean code, enhanced UX
