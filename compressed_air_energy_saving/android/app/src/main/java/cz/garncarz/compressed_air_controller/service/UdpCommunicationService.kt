package cz.garncarz.compressed_air_controller.service

import android.app.Service
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import android.util.Log
import androidx.lifecycle.MutableLiveData
import cz.garncarz.compressed_air_controller.model.ArduinoSystemState
import cz.garncarz.compressed_air_controller.model.ConnectionStatus
import cz.garncarz.compressed_air_controller.model.NetworkInfo
import cz.garncarz.compressed_air_controller.network.NetworkDiscoveryService
import cz.garncarz.compressed_air_controller.network.UdpCommunicationManager
import cz.garncarz.compressed_air_controller.parser.ArduinoDataParser
import kotlinx.coroutines.*

/**
 * Background Service - Component #6 from README
 * Maintain UDP listening and data parsing when app is backgrounded
 */
class UdpCommunicationService : Service() {

    companion object {
        private const val TAG = "UdpService"
    }

    // Binder for activity communication
    inner class LocalBinder : Binder() {
        fun getService(): UdpCommunicationService = this@UdpCommunicationService
    }

    private val binder = LocalBinder()
    private val serviceScope = CoroutineScope(Dispatchers.Main + SupervisorJob())

    // Core components
    private lateinit var udpManager: UdpCommunicationManager
    private lateinit var networkService: NetworkDiscoveryService
    private lateinit var dataParser: ArduinoDataParser

    // Live data for UI updates
    val systemState = MutableLiveData<ArduinoSystemState>()
    val networkInfo = MutableLiveData<NetworkInfo?>()
    val connectionStatus = MutableLiveData<ConnectionStatus>()
    val logMessages = MutableLiveData<List<String>>()

    // Internal state
    private var currentSystemState = ArduinoSystemState()
    private val messageLog = mutableListOf<String>()
    private val maxLogSize = 1000

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "Service created")
        
        // Initialize components
        udpManager = UdpCommunicationManager()
        networkService = NetworkDiscoveryService(this)
        dataParser = ArduinoDataParser()

        // Set up UDP message handling
        udpManager.onMessageReceived = { message ->
            handleArduinoMessage(message)
        }

        udpManager.onConnectionError = { error ->
            Log.e(TAG, "UDP error", error)
            updateConnectionStatus(ConnectionStatus.ERROR)
        }

        // Initialize live data
        systemState.value = currentSystemState
        connectionStatus.value = ConnectionStatus.DISCONNECTED
        logMessages.value = emptyList()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.d(TAG, "Service started")
        startNetworkDiscoveryAndListening()
        startPeriodicStatusRequests()
        return START_STICKY // Restart if killed by system
    }

    override fun onBind(intent: Intent): IBinder = binder

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "Service destroyed")
        cleanup()
    }

    /**
     * Start periodic STATUS requests to prevent data misalignment
     */
    private fun startPeriodicStatusRequests() {
        serviceScope.launch {
            while (true) {
                delay(60000) // Wait 1 minute
                if (currentSystemState.connectionStatus == ConnectionStatus.CONNECTED) {
                    sendCommandAsync("STATUS")
                    addLogMessage("Periodic status check...")
                }
            }
        }
    }

    /**
     * Start network discovery and UDP listening
     */
    private fun startNetworkDiscoveryAndListening() {
        serviceScope.launch {
            try {
                // Try to discover current network
                val currentNetwork = discoverNetwork()
                networkInfo.value = currentNetwork
                
                if (currentNetwork != null) {
                    // Update system state with network info
                    currentSystemState = currentSystemState.copy(
                        networkInfo = currentNetwork,
                        connectionStatus = ConnectionStatus.CONNECTING
                    )
                    systemState.value = currentSystemState
                    updateConnectionStatus(ConnectionStatus.CONNECTING)
                    
                    Log.d(TAG, "Network discovered - Listening on ${currentNetwork.localIp}, broadcast: ${currentNetwork.broadcastAddress}")
                } else {
                    Log.w(TAG, "Network discovery failed - starting UDP listening anyway")
                    updateConnectionStatus(ConnectionStatus.CONNECTING)
                }
                
                // Start UDP listening regardless of network discovery results
                udpManager.startListening(currentNetwork)
                Log.d(TAG, "UDP listening started on port 1768")
                
            } catch (e: Exception) {
                Log.e(TAG, "Error starting network services", e)
                updateConnectionStatus(ConnectionStatus.ERROR)
            }
        }
    }

    /**
     * Discover current network configuration
     */
    private suspend fun discoverNetwork(): NetworkInfo? = withContext(Dispatchers.IO) {
        try {
            // Check if we have required permissions
            if (!networkService.hasRequiredPermissions()) {
                Log.w(TAG, "Missing network permissions")
                return@withContext null
            }

            // Try regular WiFi network first
            var networkInfo = networkService.getCurrentNetworkInfo()
            
            // If no regular WiFi, check for hotspot
            if (networkInfo == null && networkService.isHotspotActive()) {
                networkInfo = networkService.getHotspotNetworkInfo()
                Log.d(TAG, "Using hotspot network")
            }
            
            networkInfo?.let { info ->
                Log.d(TAG, "Network discovered - IP: ${info.localIp}, Broadcast: ${info.broadcastAddress}")
            }
            
            networkInfo
        } catch (e: Exception) {
            Log.e(TAG, "Network discovery error", e)
            null
        }
    }

    /**
     * Handle incoming Arduino messages
     */
    private fun handleArduinoMessage(message: String) {
        serviceScope.launch {
            try {
                // Add to log
                addLogMessage(message)
                
                // Parse message and update system state
                currentSystemState = dataParser.parseMessage(message, currentSystemState)
                
                // Update connection status if we received a message
                if (currentSystemState.connectionStatus != ConnectionStatus.CONNECTED) {
                    currentSystemState = currentSystemState.copy(
                        connectionStatus = ConnectionStatus.CONNECTED
                    )
                    updateConnectionStatus(ConnectionStatus.CONNECTED)
                    
                    // Automatically request status when connection is first established
                    serviceScope.launch {
                        delay(500) // Small delay to ensure connection is stable
                        sendCommandAsync("STATUS")
                        addLogMessage("Auto-requesting system status...")
                    }
                }
                
                // Notify UI
                systemState.value = currentSystemState
                
                Log.d(TAG, "Updated state - Mode: ${currentSystemState.systemMode}, Barrels: ${currentSystemState.barrels.size}")
                
            } catch (e: Exception) {
                Log.e(TAG, "Error handling Arduino message: $message", e)
            }
        }
    }

    /**
     * Add message to log with size limit
     */
    private fun addLogMessage(message: String) {
        synchronized(messageLog) {
            // Filter out Unix timestamps that start with numbers followed by colon
            val cleanMessage = message.replaceFirst(Regex("^\\d+:\\s*"), "")
            messageLog.add(cleanMessage)
            
            // Keep log size manageable
            while (messageLog.size > maxLogSize) {
                messageLog.removeAt(0)
            }
            
            logMessages.value = messageLog.toList()
        }
    }

    /**
     * Update connection status
     */
    private fun updateConnectionStatus(status: ConnectionStatus) {
        currentSystemState = currentSystemState.copy(connectionStatus = status)
        connectionStatus.value = status
        systemState.value = currentSystemState
    }

    /**
     * Send command to Arduino (synchronous wrapper for UI)
     */
    fun sendCommandAsync(command: String) {
        serviceScope.launch {
            sendCommand(command)
        }
    }

    /**
     * Send command to Arduino
     */
    suspend fun sendCommand(command: String): Boolean {
        return try {
            val network = networkInfo.value
            if (network != null) {
                val success = udpManager.sendCommand(command, networkInfo = network)
                
                if (success) {
                    addLogMessage("SENT: $command")
                    Log.d(TAG, "Command sent successfully: $command")
                } else {
                    Log.w(TAG, "Failed to send command: $command")
                }
                return success
            } else {
                // Fallback: send to broadcast address even without network discovery
                Log.d(TAG, "No network info available - using fallback broadcast")
                val success = udpManager.sendCommand(command, networkInfo = null)
                
                if (success) {
                    addLogMessage("SENT (broadcast): $command")
                    Log.d(TAG, "Command sent via broadcast: $command")
                } else {
                    Log.w(TAG, "Failed to send command via broadcast: $command")
                }
                return success
            }
            
        } catch (e: Exception) {
            Log.e(TAG, "Error sending command: $command", e)
            false
        }
    }

    /**
     * Send barrel control command
     */
    suspend fun sendBarrelCommand(barrelId: Int, state: String): Boolean {
        return sendCommand("CMD BARREL$barrelId $state")
    }

    /**
     * Send mode change command
     */
    suspend fun sendModeCommand(mode: String): Boolean {
        return sendCommand("MODE $mode")
    }

    /**
     * Request system status
     */
    suspend fun requestStatus(): Boolean {
        return sendCommand("STATUS")
    }

    /**
     * Restart network discovery
     */
    fun restartNetworking() {
        serviceScope.launch {
            Log.d(TAG, "Restarting networking")
            udpManager.stopListening()
            delay(1000) // Brief pause
            startNetworkDiscoveryAndListening()
        }
    }

    /**
     * Get current system state
     */
    fun getCurrentState(): ArduinoSystemState = currentSystemState

    /**
     * Get the current Arduino IP address
     */
    fun getArduinoIp(): String? {
        return networkInfo.value?.arduinoIp
    }

    /**
     * Clean up resources
     */
    private fun cleanup() {
        udpManager.cleanup()
        serviceScope.cancel()
    }
}
