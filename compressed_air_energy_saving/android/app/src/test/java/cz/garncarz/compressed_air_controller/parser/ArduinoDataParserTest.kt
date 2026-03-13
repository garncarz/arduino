package cz.garncarz.compressed_air_controller.parser

import cz.garncarz.compressed_air_controller.model.*
import org.junit.Test
import org.junit.Assert.*
import org.junit.Before

/**
 * Unit tests for ArduinoDataParser - ensures mode detection and status parsing work correctly
 */
class ArduinoDataParserTest {
    
    private lateinit var parser: ArduinoDataParser
    private lateinit var initialState: ArduinoSystemState

    @Before
    fun setUp() {
        parser = ArduinoDataParser()
        initialState = ArduinoSystemState()
    }

    @Test
    fun `parseMessage should detect MODE AUTO command response`() {
        val message = "System set to AUTO mode"
        val result = parser.parseMessage(message, initialState)
        
        assertEquals(SystemMode.AUTO, result.systemMode)
    }

    @Test
    fun `parseMessage should detect MODE MANUAL command response`() {
        val message = "System set to MANUAL mode"
        val result = parser.parseMessage(message, initialState)
        
        assertEquals(SystemMode.MANUAL, result.systemMode)
    }

    @Test
    fun `parseMessage should detect STATUS command AUTO response`() {
        val message = "Mode: AUTO"
        val result = parser.parseMessage(message, initialState)
        
        assertEquals(SystemMode.AUTO, result.systemMode)
    }

    @Test
    fun `parseMessage should detect STATUS command MANUAL response`() {
        val message = "Mode: MANUAL"
        val result = parser.parseMessage(message, initialState)
        
        assertEquals(SystemMode.MANUAL, result.systemMode)
    }

    @Test
    fun `parseMessage should handle STATUS response with extra whitespace`() {
        val message = "Mode:   MANUAL  "
        val result = parser.parseMessage(message, initialState)
        
        assertEquals(SystemMode.MANUAL, result.systemMode)
    }

    @Test
    fun `parseMessage should handle case-insensitive mode detection`() {
        val testCases = listOf(
            "mode: manual" to SystemMode.MANUAL,
            "MODE: AUTO" to SystemMode.AUTO,
            "Mode: Manual" to SystemMode.MANUAL,
            "system set to manual mode" to SystemMode.MANUAL,
            "SYSTEM SET TO AUTO MODE" to SystemMode.AUTO
        )

        testCases.forEach { (message, expectedMode) ->
            val result = parser.parseMessage(message, initialState)
            assertEquals("Failed for message: '$message'", expectedMode, result.systemMode)
        }
    }

    @Test
    fun `parseMessage should detect auto-switching to manual`() {
        val message = "Auto-switching to MANUAL mode"
        val result = parser.parseMessage(message, initialState)
        
        assertEquals(SystemMode.MANUAL, result.systemMode)
    }

    @Test
    fun `parseMessage should parse barrel status line correctly`() {
        val message = "Barrel0:INTAKE (P:250 U:1 L:0)"
        val result = parser.parseMessage(message, initialState)
        
        assertTrue(result.barrels.containsKey(0))
        val barrel = result.barrels[0]!!
        assertEquals(BarrelState.INTAKE, barrel.state)
        assertEquals(250.0, barrel.pressureReading ?: 0.0, 0.01)
        assertEquals(true, barrel.upperWaterSensor)
        assertEquals(false, barrel.lowerWaterSensor)
    }

    @Test
    fun `parseMessage should parse multiple barrel status line`() {
        val message = "Barrel0:WORK (P:385 U:0 L:0) | Barrel1:WAIT_FOR_WORK (P:375 U:1 L:1)"
        val result = parser.parseMessage(message, initialState)
        
        assertEquals(2, result.barrels.size)
        
        val barrel0 = result.barrels[0]!!
        assertEquals(BarrelState.WORK, barrel0.state)
        assertEquals(385.0, barrel0.pressureReading ?: 0.0, 0.1)
        assertEquals(false, barrel0.upperWaterSensor)
        assertEquals(false, barrel0.lowerWaterSensor)
        
        val barrel1 = result.barrels[1]!!
        assertEquals(BarrelState.WAIT_FOR_WORK, barrel1.state)
        assertEquals(375.0, barrel1.pressureReading ?: 0.0, 0.1)
        assertEquals(true, barrel1.upperWaterSensor)
        assertEquals(true, barrel1.lowerWaterSensor)
    }

    @Test
    fun `parseMessage should update connection status and heartbeat`() {
        val beforeTime = System.currentTimeMillis()
        val message = "Barrel0:INTAKE (P:250 U:1 L:0)"
        val result = parser.parseMessage(message, initialState)
        val afterTime = System.currentTimeMillis()
        
        assertTrue("Heartbeat should be updated", result.lastHeartbeat >= beforeTime)
        assertTrue("Heartbeat should be recent", result.lastHeartbeat <= afterTime)
    }

    @Test
    fun `parseMessage should ignore unrecognized messages`() {
        val message = "Some random Arduino log message"
        val result = parser.parseMessage(message, initialState)
        
        // Should only update heartbeat, not change other state
        assertEquals(SystemMode.AUTO, result.systemMode) // Default value
        assertTrue(result.barrels.isEmpty())
        assertTrue(result.lastHeartbeat > 0)
    }

    @Test
    fun `parseMessage should preserve existing state when parsing new data`() {
        val stateWithBarrel = initialState.copy(
            systemMode = SystemMode.MANUAL,
            barrels = mapOf(1 to BarrelData(1, BarrelState.WORK, 400.0))
        )
        
        val message = "Mode: AUTO"
        val result = parser.parseMessage(message, stateWithBarrel)
        
        // Mode should change but barrels should be preserved
        assertEquals(SystemMode.AUTO, result.systemMode)
        assertEquals(1, result.barrels.size)
        assertEquals(BarrelState.WORK, result.barrels[1]?.state)
    }

    @Test
    fun `parseMessage should handle barrel state names correctly`() {
        val stateMessages = listOf(
            "Barrel0:WAIT_FOR_INTAKE (P:0 U:0 L:0)" to BarrelState.WAIT_FOR_INTAKE,
            "Barrel0:INTAKE (P:150 U:0 L:1)" to BarrelState.INTAKE,
            "Barrel0:WAIT_FOR_WORK (P:350 U:1 L:1)" to BarrelState.WAIT_FOR_WORK,
            "Barrel0:WORK (P:350 U:1 L:1)" to BarrelState.WORK,
            "Barrel0:EXHAUST (P:100 U:1 L:0)" to BarrelState.EXHAUST
        )

        stateMessages.forEach { (message, expectedState) ->
            val result = parser.parseMessage(message, initialState)
            assertEquals("Failed for message: '$message'", expectedState, result.barrels[0]?.state)
        }
    }
}
