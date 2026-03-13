package cz.garncarz.compressed_air_controller.network

import android.content.Context
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.net.wifi.WifiManager
import android.text.format.Formatter
import cz.garncarz.compressed_air_controller.model.NetworkInfo
import java.net.InetAddress
import java.net.NetworkInterface
import java.util.*

/**
 * Network Discovery Service - Component #2 from README
 * Detect current network configuration, including WiFi hotspots
 */
class NetworkDiscoveryService(private val context: Context) {

    private val wifiManager: WifiManager by lazy {
        context.applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
    }

    private val connectivityManager: ConnectivityManager by lazy {
        context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
    }

    /**
     * Get current network information including broadcast address calculation
     * Requires ACCESS_WIFI_STATE and ACCESS_FINE_LOCATION permissions
     */
    fun getCurrentNetworkInfo(): NetworkInfo? {
        return try {
            // Check if we're connected to WiFi
            if (!isConnectedToWifi()) {
                return null
            }

            // Get DHCP info from WiFi manager
            val dhcpInfo = wifiManager.dhcpInfo
            
            // Convert IP addresses from int to string format
            val localIp = formatIpAddress(dhcpInfo.ipAddress)
            val subnetMask = formatIpAddress(dhcpInfo.netmask)
            
            // Calculate broadcast address: ip | (~netmask)
            val broadcastInt = dhcpInfo.ipAddress or dhcpInfo.netmask.inv()
            val broadcastAddress = formatIpAddress(broadcastInt)

            NetworkInfo(
                localIp = localIp,
                broadcastAddress = broadcastAddress,
                subnetMask = subnetMask
            )
        } catch (e: Exception) {
            // Fallback to alternative method if DHCP info fails
            getNetworkInfoAlternative()
        }
    }

    /**
     * Check if device is connected to WiFi network
     */
    private fun isConnectedToWifi(): Boolean {
        return if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            val network = connectivityManager.activeNetwork ?: return false
            val capabilities = connectivityManager.getNetworkCapabilities(network) ?: return false
            capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)
        } else {
            @Suppress("DEPRECATION")
            val networkInfo = connectivityManager.activeNetworkInfo
            networkInfo?.type == ConnectivityManager.TYPE_WIFI && networkInfo.isConnected
        }
    }

    /**
     * Alternative method to get network info using NetworkInterface
     * Used when WiFi manager method fails
     */
    private fun getNetworkInfoAlternative(): NetworkInfo? {
        return try {
            val interfaces = Collections.list(NetworkInterface.getNetworkInterfaces())
            
            for (networkInterface in interfaces) {
                if (!networkInterface.isUp || networkInterface.isLoopback) continue
                
                val addresses = Collections.list(networkInterface.inetAddresses)
                for (address in addresses) {
                    if (!address.isLoopbackAddress && address is java.net.Inet4Address) {
                        val ip = address.hostAddress ?: continue
                        
                        // Calculate network info from interface
                        val interfaceAddresses = networkInterface.interfaceAddresses
                        for (interfaceAddress in interfaceAddresses) {
                            if (interfaceAddress.address == address) {
                                val prefixLength = interfaceAddress.networkPrefixLength
                                val subnet = calculateSubnetMask(prefixLength.toInt())
                                val broadcast = calculateBroadcastAddress(ip, subnet)
                                
                                return NetworkInfo(
                                    localIp = ip,
                                    broadcastAddress = broadcast,
                                    subnetMask = subnet
                                )
                            }
                        }
                    }
                }
            }
            null
        } catch (e: Exception) {
            null
        }
    }

    /**
     * Check if device is running WiFi hotspot
     * This is a best-effort check as Android restricts hotspot detection
     */
    fun isHotspotActive(): Boolean {
        return try {
            // Check if we have a network interface that looks like a hotspot
            val interfaces = Collections.list(NetworkInterface.getNetworkInterfaces())
            
            for (networkInterface in interfaces) {
                val name = networkInterface.name.lowercase()
                // Common hotspot interface names
                if (name.contains("ap") || name.contains("wlan1") || name.contains("softap")) {
                    if (networkInterface.isUp && !networkInterface.isLoopback) {
                        val addresses = Collections.list(networkInterface.inetAddresses)
                        for (address in addresses) {
                            if (!address.isLoopbackAddress && address is java.net.Inet4Address) {
                                return true
                            }
                        }
                    }
                }
            }
            false
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Get hotspot network information if available
     */
    fun getHotspotNetworkInfo(): NetworkInfo? {
        if (!isHotspotActive()) return null
        
        return try {
            val interfaces = Collections.list(NetworkInterface.getNetworkInterfaces())
            
            for (networkInterface in interfaces) {
                val name = networkInterface.name.lowercase()
                if (name.contains("ap") || name.contains("wlan1") || name.contains("softap")) {
                    if (networkInterface.isUp && !networkInterface.isLoopback) {
                        val addresses = Collections.list(networkInterface.inetAddresses)
                        for (address in addresses) {
                            if (!address.isLoopbackAddress && address is java.net.Inet4Address) {
                                val ip = address.hostAddress ?: continue
                                
                                // Typical hotspot configuration
                                val subnet = "255.255.255.0"
                                val broadcast = calculateBroadcastAddress(ip, subnet)
                                
                                return NetworkInfo(
                                    localIp = ip,
                                    broadcastAddress = broadcast,
                                    subnetMask = subnet
                                )
                            }
                        }
                    }
                }
            }
            null
        } catch (e: Exception) {
            null
        }
    }

    /**
     * Format IP address from int to dotted decimal string
     */
    private fun formatIpAddress(ip: Int): String {
        return String.format(
            Locale.getDefault(),
            "%d.%d.%d.%d",
            ip and 0xff,
            ip shr 8 and 0xff,
            ip shr 16 and 0xff,
            ip shr 24 and 0xff
        )
    }

    /**
     * Calculate subnet mask from prefix length
     */
    private fun calculateSubnetMask(prefixLength: Int): String {
        val mask = (0xffffffff.toInt() shl (32 - prefixLength))
        return formatIpAddress(mask)
    }

    /**
     * Calculate broadcast address from IP and subnet mask
     */
    private fun calculateBroadcastAddress(ip: String, subnetMask: String): String {
        return try {
            val ipAddr = InetAddress.getByName(ip)
            val maskAddr = InetAddress.getByName(subnetMask)
            
            val ipBytes = ipAddr.address
            val maskBytes = maskAddr.address
            val broadcastBytes = ByteArray(4)
            
            for (i in 0..3) {
                broadcastBytes[i] = (ipBytes[i].toInt() or (maskBytes[i].toInt().inv())).toByte()
            }
            
            val broadcastAddr = InetAddress.getByAddress(broadcastBytes)
            broadcastAddr.hostAddress ?: "255.255.255.255"
        } catch (e: Exception) {
            "255.255.255.255"
        }
    }

    /**
     * Check if required permissions are granted
     */
    fun hasRequiredPermissions(): Boolean {
        return try {
            // Try to access WiFi state to check permissions
            wifiManager.connectionInfo
            true
        } catch (e: SecurityException) {
            false
        }
    }
}
