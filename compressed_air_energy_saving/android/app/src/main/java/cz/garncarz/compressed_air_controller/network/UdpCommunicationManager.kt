package cz.garncarz.compressed_air_controller.network

import android.util.Log
import cz.garncarz.compressed_air_controller.model.NetworkInfo
import kotlinx.coroutines.*
import java.net.*
import java.nio.charset.StandardCharsets

/**
 * UDP Communication Manager - Component #1 from README
 * Handle bidirectional UDP communication with Arduino
 */
class UdpCommunicationManager {
    
    companion object {
        private const val TAG = "UdpCommunication"
        private const val UDP_PORT = 1768  // As specified in README
        private const val BUFFER_SIZE = 1024
        private const val SOCKET_TIMEOUT_MS = 5000
    }

    private var receiveSocket: DatagramSocket? = null
    private var sendSocket: DatagramSocket? = null
    private var isListening = false
    
    // Coroutine scope for managing background operations
    private val communicationScope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    
    // Callback for received messages
    var onMessageReceived: ((String) -> Unit)? = null
    var onConnectionError: ((Exception) -> Unit)? = null

    /**
     * Start listening for UDP messages from Arduino
     * Creates a background coroutine to continuously listen
     */
    fun startListening(networkInfo: NetworkInfo? = null) {
        if (isListening) {
            Log.d(TAG, "Already listening")
            return
        }

        communicationScope.launch {
            try {
                receiveSocket = DatagramSocket(UDP_PORT).apply {
                    soTimeout = SOCKET_TIMEOUT_MS
                    reuseAddress = true
                }
                
                isListening = true
                Log.d(TAG, "Started listening on UDP port $UDP_PORT")
                
                val buffer = ByteArray(BUFFER_SIZE)
                
                while (isListening && !receiveSocket!!.isClosed) {
                    try {
                        val packet = DatagramPacket(buffer, buffer.size)
                        receiveSocket!!.receive(packet)
                        
                        val message = String(
                            packet.data, 
                            0, 
                            packet.length, 
                            StandardCharsets.UTF_8
                        ).trim()
                        
                        Log.d(TAG, "Received: $message from ${packet.address}")
                        
                        // Notify listeners on main thread
                        withContext(Dispatchers.Main) {
                            onMessageReceived?.invoke(message)
                        }
                        
                    } catch (e: SocketTimeoutException) {
                        // Timeout is normal, continue listening
                        continue
                    } catch (e: Exception) {
                        if (isListening) {
                            Log.e(TAG, "Error receiving UDP message", e)
                            withContext(Dispatchers.Main) {
                                onConnectionError?.invoke(e)
                            }
                        }
                    }
                }
                
            } catch (e: Exception) {
                Log.e(TAG, "Error starting UDP listener", e)
                withContext(Dispatchers.Main) {
                    onConnectionError?.invoke(e)
                }
            }
        }
    }

    /**
     * Stop listening for UDP messages
     */
    fun stopListening() {
        isListening = false
        receiveSocket?.close()
        receiveSocket = null
        Log.d(TAG, "Stopped UDP listening")
    }

    /**
     * Send command to Arduino
     * Supports both direct IP and broadcast sending
     */
    suspend fun sendCommand(
        command: String, 
        targetIp: String? = null, 
        networkInfo: NetworkInfo? = null
    ): Boolean = withContext(Dispatchers.IO) {
        
        return@withContext try {
            if (sendSocket == null) {
                sendSocket = DatagramSocket().apply {
                    reuseAddress = true
                    broadcast = true  // Enable broadcast sending
                }
            }

            val message = command.toByteArray(StandardCharsets.UTF_8)
            val targets = mutableListOf<String>()
            
            // Determine target addresses
            when {
                targetIp != null -> targets.add(targetIp)
                networkInfo?.arduinoIp != null -> targets.add(networkInfo.arduinoIp)
                networkInfo?.broadcastAddress != null -> targets.add(networkInfo.broadcastAddress)
                else -> {
                    // Try to determine subnet broadcast from receiving messages
                    // If we know Arduino is at 10.211.17.126, use subnet broadcast 10.211.17.255
                    targets.add("10.211.17.255")  // Subnet broadcast for 10.211.17.x network
                    targets.add("255.255.255.255")  // Fallback to limited broadcast
                }
            }

            var success = false
            
            // Send to all target addresses
            for (target in targets) {
                try {
                    val address = InetAddress.getByName(target)
                    val packet = DatagramPacket(message, message.size, address, UDP_PORT)
                    
                    sendSocket!!.send(packet)
                    Log.d(TAG, "Sent command '$command' to $target:$UDP_PORT")
                    success = true
                    
                } catch (e: Exception) {
                    Log.e(TAG, "Failed to send to $target", e)
                }
            }
            
            success
            
        } catch (e: Exception) {
            Log.e(TAG, "Error sending UDP command", e)
            withContext(Dispatchers.Main) {
                onConnectionError?.invoke(e)
            }
            false
        }
    }

    /**
     * Send MODE command (AUTO or MANUAL)
     */
    suspend fun sendModeCommand(mode: String, networkInfo: NetworkInfo? = null): Boolean {
        return sendCommand("MODE $mode", networkInfo = networkInfo)
    }

    /**
     * Send barrel control command
     * Format: CMD BARREL<n> <STATE>
     */
    suspend fun sendBarrelCommand(
        barrelId: Int, 
        state: String, 
        networkInfo: NetworkInfo? = null
    ): Boolean {
        return sendCommand("CMD BARREL$barrelId $state", networkInfo = networkInfo)
    }

    /**
     * Request system status
     */
    suspend fun requestStatus(networkInfo: NetworkInfo? = null): Boolean {
        return sendCommand("STATUS", networkInfo = networkInfo)
    }

    /**
     * Request help information
     */
    suspend fun requestHelp(networkInfo: NetworkInfo? = null): Boolean {
        return sendCommand("HELP", networkInfo = networkInfo)
    }

    /**
     * Test connectivity by sending a simple command
     */
    suspend fun testConnection(networkInfo: NetworkInfo? = null): Boolean {
        return requestStatus(networkInfo)
    }

    /**
     * Clean up resources
     */
    fun cleanup() {
        stopListening()
        sendSocket?.close()
        sendSocket = null
        communicationScope.cancel()
        Log.d(TAG, "Cleaned up UDP communication")
    }

    /**
     * Check if currently listening for messages
     */
    fun isListening(): Boolean = isListening && receiveSocket?.isClosed == false
}
