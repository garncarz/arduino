package cz.garncarz.compressed_air_controller

import android.Manifest
import android.content.*
import android.content.pm.PackageManager
import android.os.Bundle
import android.os.IBinder
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.Observer
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import cz.garncarz.compressed_air_controller.model.ArduinoSystemState
import cz.garncarz.compressed_air_controller.model.ConnectionStatus
import cz.garncarz.compressed_air_controller.service.UdpCommunicationService
import cz.garncarz.compressed_air_controller.ui.BarrelAdapter
import cz.garncarz.compressed_air_controller.ui.LogAdapter
import java.text.SimpleDateFormat
import java.util.*

class MainActivity : AppCompatActivity() {
    
    private lateinit var udpService: UdpCommunicationService
    private var serviceBound = false
    
    // UI Components
    private lateinit var autoModeButton: Button
    private lateinit var manualModeButton: Button
    private lateinit var statusButton: Button
    private lateinit var systemModeText: TextView
    
    // Barrel monitoring
    private lateinit var barrelRecyclerView: RecyclerView
    private lateinit var barrelAdapter: BarrelAdapter
    
    // Manual control
    private lateinit var manualControlCard: View
    private lateinit var barrelSpinner: Spinner
    private lateinit var stateSpinner: Spinner
    private lateinit var sendCommandButton: Button
    
    // Live log
    private lateinit var logRecyclerView: RecyclerView
    private lateinit var logAdapter: LogAdapter
    
    private val timeFormatter = SimpleDateFormat("HH:mm:ss", Locale.getDefault())
    
    private val serviceConnection = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName, service: IBinder) {
            val binder = service as UdpCommunicationService.LocalBinder
            udpService = binder.getService()
            serviceBound = true
            
            // Observe Arduino data changes
            udpService.systemState.observe(this@MainActivity, Observer { data ->
                updateUI(data)
            })
            
            // Observe connection status
            udpService.connectionStatus.observe(this@MainActivity, Observer { status ->
                updateConnectionStatus(status == ConnectionStatus.CONNECTED)
            })
            
            // Observe log messages
            udpService.logMessages.observe(this@MainActivity, Observer { messages ->
                // Clear and add all messages (service manages the list)
                logAdapter.clearLogs()
                messages.forEach { message ->
                    logAdapter.addLog(message)
                }
            })
        }
        
        override fun onServiceDisconnected(arg0: ComponentName) {
            serviceBound = false
        }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        initializeViews()
        setupRecyclerViews()
        setupClickListeners()
        checkPermissions()
    }
    
    private fun initializeViews() {
        // System control
        autoModeButton = findViewById(R.id.autoModeButton)
        manualModeButton = findViewById(R.id.manualModeButton)
        statusButton = findViewById(R.id.statusButton)
        systemModeText = findViewById(R.id.systemModeText)
        
        // Manual control
        manualControlCard = findViewById(R.id.manualControlCard)
        barrelSpinner = findViewById(R.id.barrelSpinner)
        stateSpinner = findViewById(R.id.stateSpinner)
        sendCommandButton = findViewById(R.id.sendCommandButton)
        
        // Set up spinners
        val barrelNumbers = (0..3).map { "Barrel $it" }.toTypedArray()
        val barrelAdapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, barrelNumbers)
        barrelAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        barrelSpinner.adapter = barrelAdapter
        
        val states = arrayOf("INTAKE", "WORK", "EXHAUST", "WAIT_FOR_INTAKE", "WAIT_FOR_WORK")
        val stateAdapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, states)
        stateAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        stateSpinner.adapter = stateAdapter
    }
    
    private fun setupRecyclerViews() {
        // Barrel status RecyclerView
        barrelRecyclerView = findViewById(R.id.barrelRecyclerView)
        barrelAdapter = BarrelAdapter()
        barrelRecyclerView.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = barrelAdapter
        }
        
        // Log RecyclerView
        logRecyclerView = findViewById(R.id.logRecyclerView)
        logAdapter = LogAdapter()
        logRecyclerView.apply {
            layoutManager = LinearLayoutManager(this@MainActivity).apply {
                stackFromEnd = false
                reverseLayout = false
            }
            adapter = logAdapter
        }
    }
    
    private fun setupClickListeners() {
        autoModeButton.setOnClickListener {
            if (serviceBound) {
                val command = "MODE AUTO"
                udpService.sendCommandAsync(command)
                logAdapter.addLog("TX: $command")
                updateModeButtons("AUTO")
            }
        }
        
        manualModeButton.setOnClickListener {
            if (serviceBound) {
                val command = "MODE MANUAL"
                udpService.sendCommandAsync(command)
                logAdapter.addLog("TX: $command")
                updateModeButtons("MANUAL")
            }
        }
        
        statusButton.setOnClickListener {
            if (serviceBound) {
                val command = "STATUS"
                udpService.sendCommandAsync(command)
                logAdapter.addLog("TX: $command")
            }
        }
        
        sendCommandButton.setOnClickListener { 
            if (serviceBound) {
                val selectedBarrel = barrelSpinner.selectedItemPosition
                val selectedState = stateSpinner.selectedItem.toString()
                val command = "CMD $selectedState $selectedBarrel"
                udpService.sendCommandAsync(command)
                logAdapter.addLog("TX: $command")
            }
        }
    }
    
    private fun updateUI(data: ArduinoSystemState?) {
        if (data == null) return
        
        // Update mode display
        systemModeText.text = data.systemMode.name
        
        // Update mode buttons
        updateModeButtons(data.systemMode.name)
        
        // Update barrel data - convert Map to List for adapter
        val barrelList = data.barrels.values.sortedBy { it.id }
        barrelAdapter.updateBarrels(barrelList)
        
        // Show/hide manual controls
        manualControlCard.visibility = if (data.systemMode.name == "MANUAL") 
            View.VISIBLE else View.GONE
    }
    
    private fun updateModeButtons(currentMode: String) {
        when (currentMode) {
            "AUTO" -> {
                autoModeButton.setBackgroundColor(ContextCompat.getColor(this, 
                    android.R.color.holo_blue_bright))
                manualModeButton.setBackgroundColor(ContextCompat.getColor(this, 
                    android.R.color.darker_gray))
            }
            "MANUAL" -> {
                manualModeButton.setBackgroundColor(ContextCompat.getColor(this, 
                    android.R.color.holo_blue_bright))
                autoModeButton.setBackgroundColor(ContextCompat.getColor(this, 
                    android.R.color.darker_gray))
            }
        }
    }
    
    private fun updateConnectionStatus(connected: Boolean) {
        // Connection status handling removed since UI was removed
    }
    
    private fun checkPermissions() {
        val requiredPermissions = arrayOf(
            Manifest.permission.INTERNET,
            Manifest.permission.ACCESS_NETWORK_STATE,
            Manifest.permission.ACCESS_WIFI_STATE,
            Manifest.permission.ACCESS_FINE_LOCATION
        )
        
        val missingPermissions = requiredPermissions.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }
        
        if (missingPermissions.isNotEmpty()) {
            ActivityCompat.requestPermissions(this, 
                missingPermissions.toTypedArray(), PERMISSION_REQUEST_CODE)
        } else {
            startUdpService()
        }
    }
    
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == PERMISSION_REQUEST_CODE) {
            val allPermissionsGranted = grantResults.all { 
                it == PackageManager.PERMISSION_GRANTED 
            }
            
            if (allPermissionsGranted) {
                startUdpService()
            } else {
                Toast.makeText(this, getString(R.string.error_permission), 
                    Toast.LENGTH_LONG).show()
            }
        }
    }
    
    private fun startUdpService() {
        val intent = Intent(this, UdpCommunicationService::class.java)
        startService(intent)
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE)
    }
    
    override fun onStart() {
        super.onStart()
        if (!serviceBound) {
            val intent = Intent(this, UdpCommunicationService::class.java)
            bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE)
        }
    }
    
    override fun onStop() {
        super.onStop()
        if (serviceBound) {
            unbindService(serviceConnection)
            serviceBound = false
        }
    }
    
    companion object {
        private const val PERMISSION_REQUEST_CODE = 1001
    }
}
