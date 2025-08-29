package cz.garncarz.compressed_air_controller.integration

import cz.garncarz.compressed_air_controller.model.*
import cz.garncarz.compressed_air_controller.parser.ArduinoDataParser
import org.junit.Test
import org.junit.Assert.*
import org.junit.Before

/**
 * Tests for mode switching behavior to ensure barrel states are preserved
 * when switching from AUTO to MANUAL mode
 */
class ModeSwitchingBehaviorTest {
    
    private lateinit var parser: ArduinoDataParser
    
    @Before
    fun setUp() {
        parser = ArduinoDataParser()
    }

    @Test
    fun `switching from AUTO to MANUAL should NOT revert to old manual states in UI`() {
        // ACTUAL BUG REPRODUCTION based on user feedback:
        // 1. Manual mode shows EXHAUST (old manual state stored in Android)
        // 2. Switch to AUTO mode - Arduino goes to INTAKE naturally  
        // 3. Switch to MANUAL mode - Arduino sends INTAKE (correct!)
        // 4. BUT Android UI shows EXHAUST again! (BUG!)
        // The Android app is ignoring Arduino's INTAKE and showing old local state!
        
        var currentState = ArduinoSystemState()
        
        // Step 1: System starts in AUTO mode with INTAKE state
        val autoModeMessage = "System set to AUTO mode"
        currentState = parser.parseMessage(autoModeMessage, currentState)
        
        val intakeBarrelMessage = "Barrel0:INTAKE (P:250 U:0 L:0)" // Arduino is in INTAKE
        currentState = parser.parseMessage(intakeBarrelMessage, currentState)
        
        assertEquals(SystemMode.AUTO, currentState.systemMode)
        assertEquals(BarrelState.INTAKE, currentState.barrels[0]?.state)
        
        // Step 2: Switch to MANUAL mode 
        val manualModeMessage = "System set to MANUAL mode"
        currentState = parser.parseMessage(manualModeMessage, currentState)
        assertEquals(SystemMode.MANUAL, currentState.systemMode)
        
        // Step 3: Arduino sends INTAKE state (the CORRECT current state)
        val correctStateMessage = "Barrel0:INTAKE (P:250 U:0 L:0)" // Arduino sends INTAKE!
        val finalState = parser.parseMessage(correctStateMessage, currentState)
        
        // THE REAL BUG: Android should show INTAKE (what Arduino sent)
        // But somehow the Android UI is showing EXHAUST (old manual state)!
        // This means the Android app is NOT properly updating from Arduino messages
        assertEquals(SystemMode.MANUAL, finalState.systemMode)
        assertEquals(
            "Android should display INTAKE (what Arduino actually sent), not EXHAUST (old UI state)",
            BarrelState.INTAKE, 
            finalState.barrels[0]?.state
        )
        
        // This test should PASS - the parser works correctly
        // The bug must be in the Android UI layer, not the parser!
    }

    @Test
    fun `switching from MANUAL to AUTO should not affect current barrel states`() {
        // Scenario: System is in MANUAL mode, user switches back to AUTO
        
        var currentState = ArduinoSystemState()
        
        // 1. Start in MANUAL mode with barrel in EXHAUST state
        val manualModeMessage = "System set to MANUAL mode - automatic logic disabled"
        currentState = parser.parseMessage(manualModeMessage, currentState)
        
        val exhaustBarrelMessage = "Barrel0:EXHAUST (P:100 U:0 L:1)"
        currentState = parser.parseMessage(exhaustBarrelMessage, currentState)
        
        assertEquals(SystemMode.MANUAL, currentState.systemMode)
        assertEquals(BarrelState.EXHAUST, currentState.barrels[0]?.state)
        
        // 2. Switch to AUTO mode
        val autoModeMessage = "System set to AUTO mode - automatic logic enabled"
        currentState = parser.parseMessage(autoModeMessage, currentState)
        
        assertEquals(SystemMode.AUTO, currentState.systemMode)
        // Barrel state should remain the same immediately after mode switch
        assertEquals(
            "Barrel state should not change immediately when switching to AUTO",
            BarrelState.EXHAUST, 
            currentState.barrels[0]?.state
        )
    }

    @Test
    fun `multiple mode switches should preserve states correctly`() {
        // Scenario: AUTO -> MANUAL -> AUTO -> MANUAL sequence
        
        var currentState = ArduinoSystemState()
        
        // Start with barrel in INTAKE state in AUTO mode
        val intakeMessage = "Barrel0:INTAKE (P:250 U:1 L:0)"
        currentState = parser.parseMessage(intakeMessage, currentState)
        assertEquals(SystemMode.AUTO, currentState.systemMode)
        assertEquals(BarrelState.INTAKE, currentState.barrels[0]?.state)
        
        // Switch to MANUAL - should preserve INTAKE
        val manualModeMessage = "System set to MANUAL mode - automatic logic disabled"
        currentState = parser.parseMessage(manualModeMessage, currentState)
        assertEquals(SystemMode.MANUAL, currentState.systemMode)
        assertEquals(BarrelState.INTAKE, currentState.barrels[0]?.state)
        
        // In MANUAL, user changes barrel to WORK
        val workMessage = "Barrel0:WORK (P:400 U:1 L:1)"
        currentState = parser.parseMessage(workMessage, currentState)
        assertEquals(BarrelState.WORK, currentState.barrels[0]?.state)
        
        // Switch back to AUTO - should preserve WORK
        val autoModeMessage = "System set to AUTO mode - automatic logic enabled"
        currentState = parser.parseMessage(autoModeMessage, currentState)
        assertEquals(SystemMode.AUTO, currentState.systemMode)
        assertEquals(BarrelState.WORK, currentState.barrels[0]?.state)
        
        // AUTO system changes barrel to EXHAUST
        val exhaustMessage = "Barrel0:EXHAUST (P:50 U:0 L:1)"
        currentState = parser.parseMessage(exhaustMessage, currentState)
        assertEquals(BarrelState.EXHAUST, currentState.barrels[0]?.state)
        
        // Switch to MANUAL again - should preserve EXHAUST (not revert to previous WORK)
        val manualModeMessage2 = "System set to MANUAL mode - automatic logic disabled"
        currentState = parser.parseMessage(manualModeMessage2, currentState)
        assertEquals(SystemMode.MANUAL, currentState.systemMode)
        assertEquals(
            "Should preserve current EXHAUST state, not revert to previous manual WORK state",
            BarrelState.EXHAUST, 
            currentState.barrels[0]?.state
        )
    }

    @Test
    fun `mode switching with multiple barrels should preserve all states`() {
        // Scenario: System with multiple barrels in different states
        
        var currentState = ArduinoSystemState()
        
        // Set up multiple barrels in different states in AUTO mode
        val multiBarrelMessage = "Barrel0:WORK (P:350 U:1 L:1) | Barrel1:INTAKE (P:200 U:1 L:0) | Barrel2:EXHAUST (P:80 U:0 L:1)"
        currentState = parser.parseMessage(multiBarrelMessage, currentState)
        
        assertEquals(SystemMode.AUTO, currentState.systemMode)
        assertEquals(BarrelState.WORK, currentState.barrels[0]?.state)
        assertEquals(BarrelState.INTAKE, currentState.barrels[1]?.state)
        assertEquals(BarrelState.EXHAUST, currentState.barrels[2]?.state)
        
        // Switch to MANUAL - ALL barrel states should be preserved
        val manualModeMessage = "System set to MANUAL mode - automatic logic disabled"
        currentState = parser.parseMessage(manualModeMessage, currentState)
        
        assertEquals(SystemMode.MANUAL, currentState.systemMode)
        assertEquals(
            "Barrel 0 state should be preserved", 
            BarrelState.WORK, 
            currentState.barrels[0]?.state
        )
        assertEquals(
            "Barrel 1 state should be preserved", 
            BarrelState.INTAKE, 
            currentState.barrels[1]?.state
        )
        assertEquals(
            "Barrel 2 state should be preserved", 
            BarrelState.EXHAUST, 
            currentState.barrels[2]?.state
        )
    }

    @Test
    fun `cmd auto-switch to manual should preserve current states`() {
        // Scenario: User sends CMD command while in AUTO mode
        // System should auto-switch to MANUAL but preserve current states
        
        var currentState = ArduinoSystemState()
        
        // Barrel in WAIT_FOR_WORK state in AUTO mode
        val waitWorkMessage = "Barrel0:WAIT_FOR_WORK (P:380 U:1 L:1)"
        currentState = parser.parseMessage(waitWorkMessage, currentState)
        assertEquals(SystemMode.AUTO, currentState.systemMode)
        assertEquals(BarrelState.WAIT_FOR_WORK, currentState.barrels[0]?.state)
        
        // System auto-switches to MANUAL due to CMD command
        val autoSwitchMessage = "Auto-switching to MANUAL mode for direct barrel control"
        currentState = parser.parseMessage(autoSwitchMessage, currentState)
        
        assertEquals(SystemMode.MANUAL, currentState.systemMode)
        assertEquals(
            "State should be preserved during auto-switch to MANUAL",
            BarrelState.WAIT_FOR_WORK, 
            currentState.barrels[0]?.state
        )
    }
}
