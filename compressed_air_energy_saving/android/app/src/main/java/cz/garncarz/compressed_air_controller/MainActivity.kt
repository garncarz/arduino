package cz.garncarz.compressed_air_controller

import android.Manifest
import android.content.*
import android.content.pm.PackageManager
import android.os.Bundle
import android.os.IBinder
import android.util.Log
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.Observer
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.swiperefreshlayout.widget.SwipeRefreshLayout
import com.google.android.material.card.MaterialCardView
import cz.garncarz.compressed_air_controller.model.ArduinoSystemState
import cz.garncarz.compressed_air_controller.model.ConnectionStatus
import cz.garncarz.compressed_air_controller.model.ScenarioSequence
import cz.garncarz.compressed_air_controller.model.SequenceStep
import cz.garncarz.compressed_air_controller.service.UdpCommunicationService
import cz.garncarz.compressed_air_controller.ui.BarrelAdapter
import cz.garncarz.compressed_air_controller.ui.LogAdapter
import android.os.Handler
import android.os.Looper
import java.text.SimpleDateFormat
import java.util.*
import android.view.MenuItem
import android.widget.PopupMenu

class MainActivity : AppCompatActivity() {

    private lateinit var udpService: UdpCommunicationService
    private var serviceBound = false

    // UI Components
    private lateinit var swipeRefreshLayout: SwipeRefreshLayout
    private lateinit var autoModeButton: Button
    private lateinit var manualModeButton: Button
    // Removed redundant systemModeText display

    // Barrel monitoring
    private lateinit var barrelRecyclerView: RecyclerView
    private lateinit var barrelAdapter: BarrelAdapter
    private lateinit var barrelStatusAgeText: TextView
    private val ageHandler = Handler(Looper.getMainLooper())
    private var lastHeartbeatMs: Long = 0L
    private val ageRunnable = object : Runnable {
        override fun run() {
            updateBarrelAge()
            ageHandler.postDelayed(this, 1_000L)
        }
    }

    // Manual control (removed in favor of inline control on Barrel list)
    private var manualControlCard: View? = null

    // Scenario sequence
    private lateinit var scenarioSequenceCard: MaterialCardView
    private lateinit var scenarioStatusText: TextView
    private lateinit var startSequenceButton: Button
    private lateinit var nextStepButton: Button
    private lateinit var stopSequenceButton: Button
    private var scenarioSequence: ScenarioSequence? = null
    private var isPaused: Boolean = false
    private var savedPausedStates: Map<Int, cz.garncarz.compressed_air_controller.model.BarrelState>? = null
    private val exhaustHandler = Handler(Looper.getMainLooper())
    private var exhaustSpamRunnable: Runnable? = null
    private var exhaustSpamEndAt: Long = 0L

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

    override fun onDestroy() {
        super.onDestroy()
        cancelExhaustSpam()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        initializeViews()
        setupRecyclerViews()
        setupClickListeners()
        checkPermissions()

    // Ensure initial UI reflects AUTO mode until service reports otherwise
    updateModeButtons("AUTO")
    scenarioSequenceCard.visibility = View.GONE
    manualControlCard?.visibility = View.GONE
    }

    private fun initializeViews() {
        // Pull to refresh
        swipeRefreshLayout = findViewById(R.id.swipeRefreshLayout)
        swipeRefreshLayout.setColorSchemeResources(
            android.R.color.holo_blue_bright,
            android.R.color.holo_green_light,
            android.R.color.holo_orange_light
        )

        // Configure pull-to-refresh to be less aggressive
        swipeRefreshLayout.setDistanceToTriggerSync(400) // Require longer pull
        swipeRefreshLayout.setSlingshotDistance(200) // Limit overscroll

        // System control
        autoModeButton = findViewById(R.id.autoModeButton)
        manualModeButton = findViewById(R.id.manualModeButton)
    // systemModeText removed from layout

    // Manual control card removed from layout; keep nullable for legacy code paths

        // Scenario sequence
        scenarioSequenceCard = findViewById(R.id.scenarioSequenceCard)
        scenarioStatusText = findViewById(R.id.scenarioStatusText)
        startSequenceButton = findViewById(R.id.startSequenceButton)
        nextStepButton = findViewById(R.id.nextStepButton)
        stopSequenceButton = findViewById(R.id.stopSequenceButton)

        // Debug logging
        Log.d("MainActivity", "🔍 scenarioSequenceCard found: ${scenarioSequenceCard != null}")
        Log.d("MainActivity", "🔍 scenarioStatusText found: ${scenarioStatusText != null}")
        Log.d("MainActivity", "🔍 startSequenceButton found: ${startSequenceButton != null}")
        Log.d("MainActivity", "🔍 nextStepButton found: ${nextStepButton != null}")
        Log.d("MainActivity", "🔍 stopSequenceButton found: ${stopSequenceButton != null}")

    // Deprecated manual control spinners removed
    }

    private fun setupRecyclerViews() {
    // Barrel status RecyclerView
    barrelRecyclerView = findViewById(R.id.barrelRecyclerView)
    barrelStatusAgeText = findViewById(R.id.barrelStatusAgeText)
        barrelAdapter = BarrelAdapter { barrel, anchorView ->
            // Only allow inline change in MANUAL mode
            val isManual = serviceBound && udpService.systemState.value?.systemMode?.name == "MANUAL"
            if (!isManual) {
                Toast.makeText(this, "Switch to MANUAL to change states", Toast.LENGTH_SHORT).show()
                return@BarrelAdapter
            }
            // Show a small anchored popup menu instead of a full dialog
            val popup = PopupMenu(this, anchorView)
            val items = listOf(
                cz.garncarz.compressed_air_controller.model.BarrelState.INTAKE,
                cz.garncarz.compressed_air_controller.model.BarrelState.WORK,
                cz.garncarz.compressed_air_controller.model.BarrelState.EXHAUST,
                cz.garncarz.compressed_air_controller.model.BarrelState.WAIT_FOR_INTAKE,
                cz.garncarz.compressed_air_controller.model.BarrelState.WAIT_FOR_WORK
            )
            items.forEachIndexed { index, state ->
                popup.menu.add(0, index, index, state.name)
            }
            popup.setOnMenuItemClickListener { item: MenuItem ->
                val selected = items[item.itemId]
                sendCommand("CMD ${selected.name} ${barrel.id}")
                savedPausedStates = null
                isPaused = false
                true
            }
            // If user taps outside, popup dismisses and nothing happens
            popup.setOnDismissListener {
                anchorView.isPressed = false
                anchorView.refreshDrawableState()
            }
            popup.show()
        }
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
        // Pull to refresh functionality - only when ScrollView is at top
        swipeRefreshLayout.setOnRefreshListener {
            requestStatusRefresh()
        }

        // Configure SwipeRefreshLayout to work better with nested scrollable content
        swipeRefreshLayout.setOnChildScrollUpCallback { parent, child ->
            // Return true if the child can scroll up (meaning we shouldn't allow pull-to-refresh)
            // This prevents conflicts with RecyclerView scrolling
            val scrollView = child as? ScrollView
            scrollView?.canScrollVertically(-1) == true
        }

        autoModeButton.setOnClickListener {
            // Always update UI immediately
            updateModeButtons("AUTO")
            scenarioSequenceCard.visibility = View.GONE
            manualControlCard?.visibility = View.GONE
            // Discard any saved pause state
            savedPausedStates = null
            isPaused = false
            // Reset scenario (no commands) so returning to MANUAL shows ▶️
            scenarioSequence = null
            startSequenceButton.text = "▶️"
            startSequenceButton.isEnabled = true
            nextStepButton.isEnabled = false
            scenarioStatusText.text = "Ready to start 2-barrel coordination sequence"
            cancelExhaustSpam()
            // Send command if service is available
            if (serviceBound) {
                sendCommand("MODE AUTO")
            }
        }

        manualModeButton.setOnClickListener {
            Log.d("MainActivity", "🔘 MANUAL button clicked")
            // Always update UI immediately
            updateModeButtons("MANUAL")
            Log.d("MainActivity", "🔍 Setting scenario sequence to VISIBLE")
            scenarioSequenceCard.visibility = View.VISIBLE
            manualControlCard?.visibility = View.GONE // deprecated
            Log.d("MainActivity", "✅ Scenario sequence visibility set to VISIBLE")
            // Reset scenario (no commands) when entering MANUAL after AUTO
            savedPausedStates = null
            isPaused = false
            scenarioSequence = null
            startSequenceButton.text = "▶️"
            startSequenceButton.isEnabled = true
            nextStepButton.isEnabled = false
            scenarioStatusText.text = "Ready to start 2-barrel coordination sequence"
            cancelExhaustSpam()
            // Send command if service is available
            if (serviceBound) {
                sendCommand("MODE MANUAL")
            } else {
                Log.w("MainActivity", "⚠️ Service not bound; UI updated locally")
            }
        }

    // Deprecated manual control button removed

        // Scenario sequence click listeners
        startSequenceButton.setOnClickListener {
            val running = scenarioSequence?.isRunning() == true
            when {
                !running -> {
                    Log.d("MainActivity", "🎬 Start sequence button clicked!")
                    startScenarioSequence()
                }
                running && !isPaused -> {
                    Log.d("MainActivity", "⏸️ Pause clicked")
                    pauseScenario()
                }
                else -> {
                    Log.d("MainActivity", "▶️ Resume clicked")
                    resumeScenario()
                }
            }
        }

        nextStepButton?.setOnClickListener {
            Log.d("MainActivity", "⏭️ Next step button clicked!")
            if (!isPaused) {
                nextSequenceStep()
            } else {
                logAdapter.addLog("⏸️ Paused; Next is disabled")
            }
        }

    stopSequenceButton?.setOnClickListener {
            Log.d("MainActivity", "⏹️ Stop sequence button clicked!")
            stopScenarioSequence()
        }
    }

    private fun requestStatusRefresh() {
        if (serviceBound) {
            val command = "STATUS"
            sendCommand(command)
            logAdapter.addLog("🔄 Pull-to-refresh: Requesting status...")

            // Stop the refresh animation after 2 seconds or when we get data
            Handler(Looper.getMainLooper()).postDelayed({
                swipeRefreshLayout.isRefreshing = false
            }, 2000)
        } else {
            // If service isn't bound, just stop the animation
            swipeRefreshLayout.isRefreshing = false
            logAdapter.addLog("⚠️ Service not connected - cannot refresh")
        }
    }

    private fun sendCommand(command: String) {
        // Always log intent to send, even if service isn't bound yet (useful for tests)
        logAdapter.addLog("TX: $command")
        if (serviceBound) {
            udpService.sendCommandAsync(command)
        }
    }

    private fun startScenarioSequence() {
        Log.d("MainActivity", "🚀 Starting scenario sequence...")

        val available = getAvailableBarrelCount()
        Log.d("MainActivity", "Available barrel count: $available")

        if (available < 2) {
            logAdapter.addLog("⚠️ Scenario requires at least 2 barrels. Current: $available")
            return
        }

        try {
            // Always run the optimized 2-barrel scenario using barrels 0 and 1
            scenarioSequence = ScenarioSequence(2)
            val success = scenarioSequence?.startSequence() ?: false

            Log.d("MainActivity", "Scenario sequence started: $success")

            if (success) {
                val firstStep = scenarioSequence?.nextStep()
                firstStep?.let { step ->
                    Log.d("MainActivity", "First step: ${step.description}")
                    // Send UDP commands for each barrel state
                    step.barrelStates.forEach { (barrelIndex, state) ->
                        val command = "CMD ${state.name} $barrelIndex"
                        Log.d("MainActivity", "Sending command: $command")
                        sendCommand(command)
                    }
                    updateScenarioUI(step)
                    logAdapter.addLog("🎬 Started scenario: ${scenarioSequence?.getSequenceInfo()}")
                }

                // Update button states - Start button becomes Stop button
                isPaused = false
                startSequenceButton.text = "⏸️"
                startSequenceButton.isEnabled = true
                nextStepButton.isEnabled = true
                stopSequenceButton.isEnabled = true
                cancelExhaustSpam()

                Log.d("MainActivity", "Button states updated")
            }
        } catch (e: Exception) {
            Log.e("MainActivity", "Error starting scenario: ${e.message}", e)
            logAdapter.addLog("❌ Error: ${e.message}")
        }
    }

    private fun nextSequenceStep() {
        val nextStep = scenarioSequence?.nextStep()
        nextStep?.let { step ->
            // Send UDP commands for each barrel state
            step.barrelStates.forEach { (barrelIndex, state) ->
                sendCommand("CMD ${state.name} $barrelIndex")
            }
            updateScenarioUI(step)
            logAdapter.addLog("⏭️ Advanced to step ${step.stepNumber}: ${step.description}")
        } ?: run {
            logAdapter.addLog("⚠️ No sequence running. Tap ▶️ Start first.")
            nextStepButton.isEnabled = false
        }
    }

    private fun stopScenarioSequence() {
        val emergencyStates = scenarioSequence?.stopSequence()
        emergencyStates?.let { states ->
            // Send emergency EXHAUST commands
            states.forEach { (barrelIndex, state) ->
                sendCommand("CMD ${state.name} $barrelIndex")
            }
            logAdapter.addLog("⏹️ Emergency stop: all barrels set to EXHAUST")
        } ?: run {
            logAdapter.addLog("ℹ️ Scenario not running; nothing to stop")
        }

        // Reset UI
        updateScenarioUI(null)
    startSequenceButton.text = "▶️"
    startSequenceButton.isEnabled = true
    nextStepButton.isEnabled = false
    stopSequenceButton.isEnabled = true
        scenarioStatusText?.text = "Ready to start 2-barrel coordination sequence"

        logAdapter.addLog("🔄 Scenario sequence stopped")

        // Begin one-minute EXHAUST spam (once per second)
        startExhaustSpam()
        // Clear pause state
        isPaused = false
        savedPausedStates = null
    }

    private fun updateScenarioUI(step: SequenceStep?) {
        step?.let {
            val progress = scenarioSequence?.getProgressString() ?: "Ready"
            val statesText = "B0=${it.barrelStates[0]?.name ?: "-"}, B1=${it.barrelStates[1]?.name
                ?: "-"}"
            val statusText = if (progress != "Ready") {
                "Step ${it.stepNumber}/8: ${it.description}\n($statesText)"
            } else {
                "Ready"
            }
            Log.d("MainActivity", "Updating scenario UI: $statusText")
            scenarioStatusText.text = statusText
        } ?: run {
            scenarioStatusText.text = "Stopped"
        }
    }

    private fun pauseScenario() {
        // Save current states from service if available, else from last sequence step
        savedPausedStates = getCurrentBarrelStates()
        // Derive safe paused states
        val pauseStates: Map<Int, cz.garncarz.compressed_air_controller.model.BarrelState> = (0..1).associateWith { idx ->
            val cur = savedPausedStates?.get(idx)
            if (cur == cz.garncarz.compressed_air_controller.model.BarrelState.EXHAUST) {
                cz.garncarz.compressed_air_controller.model.BarrelState.WAIT_FOR_INTAKE
            } else {
                cz.garncarz.compressed_air_controller.model.BarrelState.WAIT_FOR_WORK
            }
        }
        // Send pause states
        pauseStates.forEach { (barrelIndex, state) ->
            sendCommand("CMD ${state.name} $barrelIndex")
        }
        isPaused = true
        startSequenceButton.text = "▶️"
        nextStepButton.isEnabled = false
        logAdapter.addLog("⏸️ Paused scenario; barrels set to WAIT_*")
    }

    private fun resumeScenario() {
        // Restore saved states if any
        val toRestore = savedPausedStates
        if (toRestore != null) {
            toRestore.forEach { (barrelIndex, state) ->
                sendCommand("CMD ${state.name} $barrelIndex")
            }
            logAdapter.addLog("▶️ Resumed scenario; restored previous states")
        } else {
            logAdapter.addLog("▶️ Resumed scenario")
        }
        isPaused = false
        startSequenceButton.text = "⏸️"
        nextStepButton.isEnabled = true
        savedPausedStates = null
        cancelExhaustSpam()
    }

    private fun getCurrentBarrelStates(): Map<Int, cz.garncarz.compressed_air_controller.model.BarrelState> {
        if (serviceBound) {
            udpService.systemState.value?.barrels?.let { map ->
                val out = mutableMapOf<Int, cz.garncarz.compressed_air_controller.model.BarrelState>()
                for (i in 0..1) {
                    map[i]?.let { out[i] = it.state }
                }
                if (out.isNotEmpty()) return out
            }
        }
        scenarioSequence?.getCurrentStep()?.barrelStates?.let { stepMap ->
            val out = mutableMapOf<Int, cz.garncarz.compressed_air_controller.model.BarrelState>()
            for (i in 0..1) {
                stepMap[i]?.let { out[i] = it }
            }
            if (out.isNotEmpty()) return out
        }
        return emptyMap()
    }

    private fun startExhaustSpam() {
        cancelExhaustSpam()
        exhaustSpamEndAt = System.currentTimeMillis() + 60_000L
        exhaustSpamRunnable = object : Runnable {
            override fun run() {
                // Send EXHAUST to barrels 0 and 1
                for (i in 0..1) {
                    sendCommand("CMD ${cz.garncarz.compressed_air_controller.model.BarrelState.EXHAUST.name} $i")
                }
                if (System.currentTimeMillis() < exhaustSpamEndAt) {
                    exhaustHandler.postDelayed(this, 1_000L)
                } else {
                    logAdapter.addLog("✅ Finished EXHAUST spam (1 minute)")
                }
            }
        }
        exhaustHandler.post(exhaustSpamRunnable!!)
        logAdapter.addLog("⏱️ Spamming EXHAUST every second for 1 minute…")
    }

    private fun cancelExhaustSpam() {
        exhaustSpamRunnable?.let { exhaustHandler.removeCallbacks(it) }
        exhaustSpamRunnable = null
        exhaustSpamEndAt = 0L
    }

    private fun getAvailableBarrelCount(): Int {
        // Use reported barrel count when available; default to 2 for tests/emulator.
        val reported = if (serviceBound) {
            udpService.systemState.value?.getBarrelCount() ?: 0
        } else 0
        return maxOf(2, reported)
    }

    private fun updateUI(data: ArduinoSystemState?) {
        if (data == null) return

        // Stop pull-to-refresh animation when new data arrives
        if (swipeRefreshLayout.isRefreshing) {
            swipeRefreshLayout.isRefreshing = false
        }

    // Mode text removed; buttons reflect active mode

        // Track last heartbeat for age display
        lastHeartbeatMs = data.lastHeartbeat

        // Update mode buttons
        updateModeButtons(data.systemMode.name)

        // Update barrel data - convert Map to List for adapter
        val barrelList = data.barrels.values.sortedBy { it.id }
        barrelAdapter.updateBarrels(barrelList)

        // Show/hide controls based on mode
        updateSystemUI(data)

        // Start/refresh age ticker - use a simple flag to avoid API 29 requirement
        if (barrelStatusAgeText.visibility == View.VISIBLE) {
            // Remove any existing callbacks first, then post new one
            ageHandler.removeCallbacks(ageRunnable)
            ageHandler.post(ageRunnable)
            updateBarrelAge()
        }
    }

    private fun updateBarrelAge() {
        if (!::barrelStatusAgeText.isInitialized) return
        val now = System.currentTimeMillis()
        val ageMs = (now - lastHeartbeatMs).coerceAtLeast(0L)
        val text = when {
            lastHeartbeatMs == 0L -> " (—)"
            ageMs < 1500L -> " (Just now)"
            ageMs < 60_000L -> " (${ageMs / 1000}s ago)"
            ageMs < 3_600_000L -> {
                val m = ageMs / 60_000L
                " (${m}m ago)"
            }
            else -> {
                val h = ageMs / 3_600_000L
                " (${h}h ago)"
            }
        }
        barrelStatusAgeText.text = text
    }

    private fun updateSystemUI(systemState: ArduinoSystemState) {
        when (systemState.systemMode.name) {
            "MANUAL" -> {
                // Show manual controls
                manualControlCard?.visibility = View.GONE
                // Scenario sequence card visibility is controlled by user action (Manual button)
            }
            "AUTO" -> {
                // Hide manual controls
                manualControlCard?.visibility = View.GONE
            }
        }
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
                Toast.makeText(this, "Permissions required for network communication",
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
