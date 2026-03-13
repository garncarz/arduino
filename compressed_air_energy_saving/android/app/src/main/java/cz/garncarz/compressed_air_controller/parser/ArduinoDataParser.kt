package cz.garncarz.compressed_air_controller.parser

import cz.garncarz.compressed_air_controller.model.*
import java.util.regex.Pattern

/**
 * Arduino Data Parser - Component #3 from README
 * Parse incoming log messages and extract system state, sensor values, and barrel information
 */
class ArduinoDataParser {

    companion object {
        // Regex patterns for parsing Arduino messages
        private val BARREL_STATE_PATTERN = Pattern.compile(
            "States?:?\\s*(.+)", Pattern.CASE_INSENSITIVE
        )
        
        private val MODE_CHANGE_PATTERN = Pattern.compile(
            "System set to (AUTO|MANUAL) mode", Pattern.CASE_INSENSITIVE
        )
        
        private val MODE_STATUS_PATTERN = Pattern.compile(
            "Mode:\\s*(AUTO|MANUAL)", Pattern.CASE_INSENSITIVE
        )
        
        private val AUTO_SWITCH_PATTERN = Pattern.compile(
            "Auto-switching to MANUAL mode", Pattern.CASE_INSENSITIVE
        )
        
        private val BARREL_COMMAND_PATTERN = Pattern.compile(
            "Barrel (\\d+) set to (\\w+)", Pattern.CASE_INSENSITIVE
        )
        
        private val PRESSURE_PATTERN = Pattern.compile(
            "Barrel(\\d+).*pressure:?\\s*([\\d.]+)", Pattern.CASE_INSENSITIVE
        )
        
        private val WATER_SENSOR_PATTERN = Pattern.compile(
            "Barrel(\\d+).*water:?\\s*(upper|lower|high|low):?\\s*(true|false|on|off|1|0)", 
            Pattern.CASE_INSENSITIVE
        )
        
        private val NETWORK_INFO_PATTERN = Pattern.compile(
            "WiFi connected.*IP:?\\s*([\\d.]+)", Pattern.CASE_INSENSITIVE
        )
        
        private val BROADCAST_INFO_PATTERN = Pattern.compile(
            "Broadcast IP:?\\s*([\\d.]+)", Pattern.CASE_INSENSITIVE
        )
        
        // Pattern for main status line: Barrel0:WORK (P:385 U:0 L:0) | Barrel1:WAIT_FOR_WORK (P:375 U:0 L:0)
        private val STATUS_LINE_PATTERN = Pattern.compile(
            "Barrel(\\d+):(\\w+)\\s*\\(P:(\\d+)\\s+U:([01])\\s+L:([01])\\)"
        )
    }

    /**
     * Parse a single Arduino log message and extract relevant data
     */
    fun parseMessage(message: String, currentState: ArduinoSystemState): ArduinoSystemState {
        var updatedState = currentState.copy(lastHeartbeat = System.currentTimeMillis())

        // Parse system mode changes
        val modeMatch = MODE_CHANGE_PATTERN.matcher(message)
        if (modeMatch.find()) {
            val mode = SystemMode.fromString(modeMatch.group(1))
            if (mode != null) {
                updatedState = updatedState.copy(systemMode = mode)
            }
        }

        // Parse system mode from STATUS command response
        val statusModeMatch = MODE_STATUS_PATTERN.matcher(message)
        if (statusModeMatch.find()) {
            val mode = SystemMode.fromString(statusModeMatch.group(1))
            if (mode != null) {
                updatedState = updatedState.copy(systemMode = mode)
            }
        }

        // Parse auto-switching to manual mode
        if (AUTO_SWITCH_PATTERN.matcher(message).find()) {
            updatedState = updatedState.copy(systemMode = SystemMode.MANUAL)
        }

        // Parse main status line format: Barrel0:WORK (P:385 U:0 L:0) | Barrel1:WAIT_FOR_WORK (P:375 U:0 L:0)
        val statusMatcher = STATUS_LINE_PATTERN.matcher(message)
        val foundBarrels = mutableMapOf<Int, BarrelData>()
        while (statusMatcher.find()) {
            val barrelId = statusMatcher.group(1).toIntOrNull()
            val stateStr = statusMatcher.group(2)
            val pressure = statusMatcher.group(3).toIntOrNull()
            val upperSensor = statusMatcher.group(4) == "1"
            val lowerSensor = statusMatcher.group(5) == "1"
            
            if (barrelId != null && pressure != null) {
                val state = BarrelState.fromString(stateStr) ?: BarrelState.WAIT_FOR_INTAKE
                foundBarrels[barrelId] = BarrelData(
                    id = barrelId,
                    state = state,
                    pressureReading = pressure.toDouble(),
                    upperWaterSensor = upperSensor,
                    lowerWaterSensor = lowerSensor
                )
            }
        }
        
        if (foundBarrels.isNotEmpty()) {
            updatedState = updatedState.copy(barrels = foundBarrels)
        }

        // Parse barrel states from status messages like "States: Barrel0:WORK, Barrel1:INTAKE"
        val stateMatch = BARREL_STATE_PATTERN.matcher(message)
        if (stateMatch.find()) {
            val statesText = stateMatch.group(1)
            val barrelUpdates = parseBarrelStates(statesText)
            if (barrelUpdates.isNotEmpty()) {
                val updatedBarrels = updatedState.barrels.toMutableMap()
                barrelUpdates.forEach { (id, barrelData) ->
                    updatedBarrels[id] = barrelData
                }
                updatedState = updatedState.copy(barrels = updatedBarrels)
            }
        }

        // Parse individual barrel commands
        val commandMatch = BARREL_COMMAND_PATTERN.matcher(message)
        if (commandMatch.find()) {
            val barrelId = commandMatch.group(1).toIntOrNull()
            val stateStr = commandMatch.group(2)
            val state = BarrelState.fromString(stateStr)
            
            if (barrelId != null && state != null) {
                val updatedBarrels = updatedState.barrels.toMutableMap()
                val currentBarrel = updatedBarrels[barrelId] ?: BarrelData(barrelId, state)
                updatedBarrels[barrelId] = currentBarrel.copy(state = state)
                updatedState = updatedState.copy(barrels = updatedBarrels)
            }
        }

        // Parse pressure readings
        val pressureMatch = PRESSURE_PATTERN.matcher(message)
        if (pressureMatch.find()) {
            val barrelId = pressureMatch.group(1).toIntOrNull()
            val pressure = pressureMatch.group(2).toDoubleOrNull()
            
            if (barrelId != null && pressure != null) {
                val updatedBarrels = updatedState.barrels.toMutableMap()
                val currentBarrel = updatedBarrels[barrelId] 
                    ?: BarrelData(barrelId, BarrelState.WAIT_FOR_INTAKE)
                updatedBarrels[barrelId] = currentBarrel.copy(pressureReading = pressure)
                updatedState = updatedState.copy(barrels = updatedBarrels)
            }
        }

        // Parse water sensor readings
        val waterMatch = WATER_SENSOR_PATTERN.matcher(message)
        if (waterMatch.find()) {
            val barrelId = waterMatch.group(1).toIntOrNull()
            val sensorType = waterMatch.group(2).lowercase()
            val sensorValue = parseBooleanValue(waterMatch.group(3))
            
            if (barrelId != null && sensorValue != null) {
                val updatedBarrels = updatedState.barrels.toMutableMap()
                val currentBarrel = updatedBarrels[barrelId] 
                    ?: BarrelData(barrelId, BarrelState.WAIT_FOR_INTAKE)
                
                val updatedBarrel = when (sensorType) {
                    "upper", "high" -> currentBarrel.copy(upperWaterSensor = sensorValue)
                    "lower", "low" -> currentBarrel.copy(lowerWaterSensor = sensorValue)
                    else -> currentBarrel
                }
                updatedBarrels[barrelId] = updatedBarrel
                updatedState = updatedState.copy(barrels = updatedBarrels)
            }
        }

        // Parse network information
        val networkMatch = NETWORK_INFO_PATTERN.matcher(message)
        if (networkMatch.find()) {
            val arduinoIp = networkMatch.group(1)
            val currentNetwork = updatedState.networkInfo
            val updatedNetwork = currentNetwork?.copy(arduinoIp = arduinoIp)
                ?: NetworkInfo("", "", "", arduinoIp)
            updatedState = updatedState.copy(
                networkInfo = updatedNetwork,
                connectionStatus = ConnectionStatus.CONNECTED
            )
        }

        val broadcastMatch = BROADCAST_INFO_PATTERN.matcher(message)
        if (broadcastMatch.find()) {
            val broadcastIp = broadcastMatch.group(1)
            val currentNetwork = updatedState.networkInfo
            if (currentNetwork != null) {
                val updatedNetwork = currentNetwork.copy(broadcastAddress = broadcastIp)
                updatedState = updatedState.copy(networkInfo = updatedNetwork)
            }
        }

        return updatedState
    }

    /**
     * Parse barrel states from text like "Barrel0:WORK, Barrel1:INTAKE"
     */
    private fun parseBarrelStates(statesText: String): Map<Int, BarrelData> {
        val barrelUpdates = mutableMapOf<Int, BarrelData>()
        
        // Split by comma and parse each barrel state
        statesText.split(",").forEach { barrelState ->
            val parts = barrelState.trim().split(":")
            if (parts.size == 2) {
                // Extract barrel number from "Barrel0" -> 0
                val barrelIdStr = parts[0].trim().removePrefix("Barrel")
                val barrelId = barrelIdStr.toIntOrNull()
                val state = BarrelState.fromString(parts[1].trim())
                
                if (barrelId != null && state != null) {
                    barrelUpdates[barrelId] = BarrelData(barrelId, state)
                }
            }
        }
        
        return barrelUpdates
    }

    /**
     * Parse boolean values from various string representations
     */
    private fun parseBooleanValue(value: String): Boolean? {
        return when (value.lowercase()) {
            "true", "on", "1" -> true
            "false", "off", "0" -> false
            else -> null
        }
    }
}
