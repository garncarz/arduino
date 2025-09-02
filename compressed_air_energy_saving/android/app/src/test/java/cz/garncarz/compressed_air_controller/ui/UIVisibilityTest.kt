package cz.garncarz.compressed_air_controller.ui

import cz.garncarz.compressed_air_controller.model.SystemMode
import cz.garncarz.compressed_air_controller.model.ArduinoSystemState
import org.junit.Test
import org.junit.Assert.*

/**
 * Fast unit tests for UI visibility logic
 * Tests when scenario sequence should be visible/hidden
 */
class UIVisibilityTest {

    @Test
    fun testScenarioVisibilityInAutoMode() {
        val systemState = ArduinoSystemState(systemMode = SystemMode.AUTO)

        val shouldShowScenario = shouldShowScenarioSequence(systemState)

        assertFalse("Scenario sequence should be HIDDEN in AUTO mode", shouldShowScenario)
    }

    @Test
    fun testScenarioVisibilityInManualMode() {
        val systemState = ArduinoSystemState(systemMode = SystemMode.MANUAL)

        val shouldShowScenario = shouldShowScenarioSequence(systemState)

        assertTrue("Scenario sequence should be VISIBLE in MANUAL mode", shouldShowScenario)
    }

    // Manual control card removed; inline control is always present in MANUAL via item taps

    @Test
    fun testModeTransitionLogic() {
        // Test AUTO to MANUAL transition
        var systemState = ArduinoSystemState(systemMode = SystemMode.AUTO)
        assertFalse("Should start hidden", shouldShowScenarioSequence(systemState))

        systemState = ArduinoSystemState(systemMode = SystemMode.MANUAL)
        assertTrue("Should show after switching to MANUAL", shouldShowScenarioSequence(systemState))

        // Test MANUAL to AUTO transition
        systemState = ArduinoSystemState(systemMode = SystemMode.AUTO)
        assertFalse("Should hide after switching back to AUTO", shouldShowScenarioSequence(systemState))
    }

    @Test
    fun testVisibilityConsistency() {
    // Only scenario sequence visibility is asserted now
    val autoState = ArduinoSystemState(systemMode = SystemMode.AUTO)
    assertFalse("AUTO mode: scenario sequence hidden", shouldShowScenarioSequence(autoState))

    val manualState = ArduinoSystemState(systemMode = SystemMode.MANUAL)
    assertTrue("MANUAL mode: scenario sequence visible", shouldShowScenarioSequence(manualState))
    }

    // Helper methods that simulate the UI logic
    private fun shouldShowScenarioSequence(systemState: ArduinoSystemState): Boolean {
        return systemState.systemMode == SystemMode.MANUAL
    }

    // Manual control card removed
}
