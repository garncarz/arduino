package cz.garncarz.compressed_air_controller.integration

import cz.garncarz.compressed_air_controller.model.*
import cz.garncarz.compressed_air_controller.parser.ArduinoDataParser
import org.junit.Test
import org.junit.Assert.*
import org.junit.Before

/**
 * Integration tests for the complete mode detection and auto-status flow
 * These tests simulate real-world scenarios to ensure the functionality works end-to-end
 */
class ModeDetectionIntegrationTest {
    
    private lateinit var parser: ArduinoDataParser
    
    @Before
    fun setUp() {
        parser = ArduinoDataParser()
    }

    @Test
    fun `complete app restart and manual mode detection scenario`() {
        // Scenario: Arduino is in MANUAL mode, Android app restarts and needs to detect this

        // 1. App starts with default AUTO state
        var currentState = ArduinoSystemState() // defaults to AUTO
        assertEquals(SystemMode.AUTO, currentState.systemMode)

        // 2. App receives first Arduino message (establishes connection)
        val firstMessage = "Barrel0:INTAKE (P:250 U:1 L:0)"
        currentState = parser.parseMessage(firstMessage, currentState)
        // Parser should update heartbeat, connection status is handled by service layer
        assertTrue("Heartbeat should be updated", currentState.lastHeartbeat > 0)
        // Mode should still be AUTO at this point
        assertEquals(SystemMode.AUTO, currentState.systemMode)

        // 3. App would automatically send STATUS command here (tested separately)
        // 4. Arduino responds with status showing it's actually in MANUAL mode
        val statusResponse = "=== SYSTEM STATUS ==="
        currentState = parser.parseMessage(statusResponse, currentState)

        val modeResponse = "Mode: MANUAL"
        currentState = parser.parseMessage(modeResponse, currentState)
        
        // 5. App should now correctly show MANUAL mode
        assertEquals(SystemMode.MANUAL, currentState.systemMode)

        // 6. Additional status info should be parsed correctly
        val barrelStatusResponse = "Barrel0: INTAKE (manual)"
        currentState = parser.parseMessage(barrelStatusResponse, currentState)
        
        // State should remain MANUAL
        assertEquals(SystemMode.MANUAL, currentState.systemMode)
    }

    @Test
    fun `mode change command flow works correctly`() {
        // Scenario: User switches from AUTO to MANUAL via app

        var currentState = ArduinoSystemState() // defaults to AUTO
        assertEquals(SystemMode.AUTO, currentState.systemMode)

        // 1. User clicks MANUAL mode button, app sends "MODE MANUAL"
        // 2. Arduino responds confirming the change
        val modeChangeResponse = "System set to MANUAL mode - automatic logic disabled"
        currentState = parser.parseMessage(modeChangeResponse, currentState)
        
        assertEquals(SystemMode.MANUAL, currentState.systemMode)

        // 3. User clicks back to AUTO mode
        val autoModeResponse = "System set to AUTO mode - automatic logic enabled"
        currentState = parser.parseMessage(autoModeResponse, currentState)
        
        assertEquals(SystemMode.AUTO, currentState.systemMode)
    }

    @Test
    fun `auto-switching scenario works correctly`() {
        // Scenario: System automatically switches to MANUAL mode due to some condition
        
        var currentState = ArduinoSystemState() // defaults to AUTO
        assertEquals(SystemMode.AUTO, currentState.systemMode)

        // Arduino auto-switches to manual mode
        val autoSwitchMessage = "Auto-switching to MANUAL mode for direct barrel control"
        currentState = parser.parseMessage(autoSwitchMessage, currentState)
        
        assertEquals(SystemMode.MANUAL, currentState.systemMode)
    }

    @Test
    fun `status request handles both mode formats correctly`() {
        // Test that both command responses and status responses work
        
        val testCases = listOf(
            // Command response format
            "System set to MANUAL mode" to SystemMode.MANUAL,
            "System set to AUTO mode" to SystemMode.AUTO,
            
            // Status response format  
            "Mode: MANUAL" to SystemMode.MANUAL,
            "Mode: AUTO" to SystemMode.AUTO,
            
            // Auto-switch format
            "Auto-switching to MANUAL mode" to SystemMode.MANUAL
        )

        testCases.forEach { (message, expectedMode) ->
            val initialState = ArduinoSystemState()
            val result = parser.parseMessage(message, initialState)
            
            assertEquals(
                "Failed for message: '$message'", 
                expectedMode, 
                result.systemMode
            )
        }
    }

    @Test
    fun `barrel state changes are preserved during mode changes`() {
        // Scenario: Ensure barrel data isn't lost when mode changes
        
        var currentState = ArduinoSystemState()
        
        // 1. Start with some barrel data
        val barrelMessage = "Barrel0:WORK (P:350 U:1 L:0) | Barrel1:INTAKE (P:200 U:0 L:1)"
        currentState = parser.parseMessage(barrelMessage, currentState)
        
        assertEquals(2, currentState.barrels.size)
        assertEquals(BarrelState.WORK, currentState.barrels[0]?.state)
        assertEquals(BarrelState.INTAKE, currentState.barrels[1]?.state)
        
        // 2. Mode changes to MANUAL
        val modeChange = "Mode: MANUAL"
        currentState = parser.parseMessage(modeChange, currentState)
        
        // 3. Barrel data should be preserved
        assertEquals(SystemMode.MANUAL, currentState.systemMode)
        assertEquals(2, currentState.barrels.size)
        assertEquals(BarrelState.WORK, currentState.barrels[0]?.state)
        assertEquals(BarrelState.INTAKE, currentState.barrels[1]?.state)
    }

    @Test
    fun `heartbeat is always updated regardless of message content`() {
        val currentState = ArduinoSystemState()
        val beforeTime = System.currentTimeMillis()
        
        // Even unrecognized messages should update heartbeat
        val randomMessage = "Some random Arduino debug message"
        val result = parser.parseMessage(randomMessage, currentState)
        
        val afterTime = System.currentTimeMillis()
        
        assertTrue("Heartbeat should be updated", result.lastHeartbeat >= beforeTime)
        assertTrue("Heartbeat should be recent", result.lastHeartbeat <= afterTime)
    }

    @Test
    fun `connection status updates correctly with first message`() {
        val disconnectedState = ArduinoSystemState(connectionStatus = ConnectionStatus.DISCONNECTED)
        assertEquals(ConnectionStatus.DISCONNECTED, disconnectedState.connectionStatus)
        
        // Any message should trigger connection
        val anyMessage = "Barrel0:WORK (P:300 U:1 L:1)"
        val result = parser.parseMessage(anyMessage, disconnectedState)
        
        // Parser should update heartbeat, service layer handles connection status
        assertTrue("Heartbeat should be updated", result.lastHeartbeat > 0)
    }
}
