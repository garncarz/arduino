# Arduino Compressed Air Energy Saving - Android Controller

Android application for monitoring and controlling the compressed air energy saving system running on Arduino Uno R4 WiFi.

## Overview

This Android app provides real-time monitoring and control of a multi-barrel compressed air energy production system. The system uses water-filled barrels to store compressed air energy and automatically manages the energy production cycle to ensure continuous power output.

## Features

### Monitoring
- **Real-time status**: Display current state of each barrel (INTAKE, WORK, EXHAUST, WAIT_FOR_WORK, WAIT_FOR_INTAKE)
- **Sensor data**: Show pressure readings and water level sensors (upper/lower) for each barrel
- **Continuous monitoring**: Parse and display sensor values in both AUTO and MANUAL modes
- **System mode**: Shows whether system is in AUTO or MANUAL mode
- **Live logs**: Stream of Arduino log messages via UDP broadcast
- **Network info**: Display Arduino IP address and connection status
- **Manual mode visibility**: Even when in manual control, all sensor readings and barrel states remain visible for monitoring

### Control
- **Mode switching**: Switch between AUTO and MANUAL operation modes
- **Manual barrel control**: Directly set individual barrel states when in manual mode
- **Automatic mode switching**: CMD commands automatically enable manual mode for convenience
- **System status**: Request current system status on demand

## Technical Details

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

### Network Requirements
- Android device and Arduino must be on the same WiFi network
- Arduino broadcasts to subnet broadcast address (more reliable than 255.255.255.255)
- App needs INTERNET permission for UDP socket access
- **WiFi Hotspot Detection**:
  - Requires `ACCESS_WIFI_STATE` and `ACCESS_FINE_LOCATION` permissions
  - On Android 6+ (API 23+), location permission is mandatory for WiFi network information
  - App can detect if phone is running a hotspot and get its network details
  - Broadcast address calculation: `phone_ip | (~subnet_mask)`

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

## Development Setup

### Prerequisites
- Android Studio
- Kotlin development environment
- Target API: Android 5.0+ (API 21+)
- Permissions:
  - `INTERNET` - Required for UDP socket access
  - `ACCESS_NETWORK_STATE` - Read network connection state
  - `ACCESS_WIFI_STATE` - Read WiFi connection details and network info
  - `ACCESS_FINE_LOCATION` - Required for WiFi network discovery on Android 6+ (API 23+)
  - `CHANGE_WIFI_STATE` - Optional: If app needs to connect to different networks

### Key Components to Implement
1. **UDP Communication Manager**: Handle bidirectional UDP communication
2. **Network Discovery Service**: Detect current network configuration, including WiFi hotspots
3. **Arduino Data Parser**: Parse incoming log messages and extract system state, sensor values, and barrel information
4. **Real-time Data Model**: Maintain current state of all barrels, sensors, and system status regardless of operation mode
5. **UI Components**:
   - System status display with real-time sensor readings
   - Manual control interface (overlay on monitoring display)
   - Real-time log viewer
   - Connection status indicator
   - Network info display (current IP, broadcast address)
   - Sensor value displays (pressure, water levels) that update continuously
6. **Background Service**: Maintain UDP listening and data parsing when app is backgrounded

### Architecture
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

## Usage

1. **Connect to WiFi**: Ensure Android device is on same network as Arduino
2. **Launch App**: Open the Arduino Controller app
3. **Auto-discover**: App will listen for Arduino broadcasts and display connection status
4. **Monitor**: View real-time barrel states, sensor readings, and system logs
   - **Continuous monitoring**: Sensor data (pressure, water levels) updates in real-time regardless of AUTO/MANUAL mode
   - **State visibility**: Current barrel states always visible even during manual control
5. **Control**:
   - Use MODE buttons to switch between AUTO/MANUAL
   - **Manual mode**: Use barrel control buttons while still seeing live sensor data
   - **Monitoring overlay**: Manual controls overlay the monitoring display so sensor values remain visible
   - Use STATUS button to request current system information

## Future Enhancements

- **Data visualization**: Charts showing energy production over time
- **Scheduling**: Automated mode switching based on time/conditions
- **Multiple Arduino support**: Manage multiple systems from one app
- **Export logs**: Save system logs for analysis
- **Push notifications**: Alerts for system issues or state changes
- **Network discovery**: Automatically find Arduino devices on network

## Related Files

- **Arduino Code**: `../compressed_air_energy_saving.ino` - Main Arduino sketch
- **WiFi Module**: `../wifi.cpp` - Arduino WiFi communication handling
- **Command Processing**: `../commands.cpp` - Arduino command parsing
- **System Logic**: `../logic.cpp` - Core barrel state management
- **Documentation**: `../README.md` - Arduino system documentation
