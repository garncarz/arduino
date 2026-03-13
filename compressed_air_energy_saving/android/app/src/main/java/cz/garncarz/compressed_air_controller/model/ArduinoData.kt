package cz.garncarz.compressed_air_controller.model

/**
 * Arduino System States as defined in README (8-state machine)
 */
enum class BarrelState {
    IDLE,             // Safe startup state - all valves closed, no automatic progression
    INIT,             // Initial air space preparation - opens INTAKE+WORK valves simultaneously
    INTAKE,           // Pressurizing the barrel with compressed air (time-limited)
    WORK,             // Generating energy by releasing air through turbine
    EXHAUST,          // Releasing remaining pressure and refilling with water
    EXIT,             // Final coordination - opens WORK+EXHAUST valves simultaneously
    WAIT_FOR_INTAKE,  // Waiting for opportunity to start preparation
    WAIT_FOR_WORK;    // Ready to work, waiting for current working barrel to finish

    companion object {
        fun fromString(state: String): BarrelState? {
            return try {
                valueOf(state.uppercase())
            } catch (e: IllegalArgumentException) {
                null
            }
        }
    }
}

/**
 * System operation mode
 */
enum class SystemMode {
    AUTO,    // System manages barrel transitions automatically
    MANUAL;  // User has direct control over individual barrel states

    companion object {
        fun fromString(mode: String): SystemMode? {
            return try {
                valueOf(mode.uppercase())
            } catch (e: IllegalArgumentException) {
                null
            }
        }
    }
}

/**
 * Data class representing a single barrel's state and sensor readings
 */
data class BarrelData(
    val id: Int,
    val state: BarrelState,
    val pressureReading: Double? = null,
    val upperWaterSensor: Boolean? = null,
    val lowerWaterSensor: Boolean? = null,
    val lastUpdate: Long = System.currentTimeMillis()
)

/**
 * Complete system state data model
 * Maintains current state of all barrels, sensors, and system status regardless of operation mode
 */
data class ArduinoSystemState(
    val systemMode: SystemMode = SystemMode.AUTO,
    val barrels: Map<Int, BarrelData> = emptyMap(),
    val networkInfo: NetworkInfo? = null,
    val connectionStatus: ConnectionStatus = ConnectionStatus.DISCONNECTED,
    val lastHeartbeat: Long = 0L
) {
    fun getBarrelCount(): Int = barrels.size

    fun getWorkingBarrel(): BarrelData? =
        barrels.values.find { it.state == BarrelState.WORK }

    fun isConnected(): Boolean =
        connectionStatus == ConnectionStatus.CONNECTED &&
        (System.currentTimeMillis() - lastHeartbeat) < HEARTBEAT_TIMEOUT_MS

    companion object {
        private const val HEARTBEAT_TIMEOUT_MS = 30000L // 30 seconds
    }
}

/**
 * Network configuration information
 */
data class NetworkInfo(
    val localIp: String,
    val broadcastAddress: String,
    val subnetMask: String,
    val arduinoIp: String? = null
)

/**
 * Connection status with Arduino
 */
enum class ConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
}
