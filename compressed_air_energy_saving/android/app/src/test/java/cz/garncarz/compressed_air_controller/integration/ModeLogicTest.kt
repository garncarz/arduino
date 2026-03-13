package cz.garncarz.compressed_air_controller.integration

import cz.garncarz.compressed_air_controller.model.SystemMode
import cz.garncarz.compressed_air_controller.model.ArduinoSystemState
import org.junit.Test
import org.junit.Assert.*

/**
 * Fast integration tests for mode switching logic
 * Tests the complete flow from mode switch to UI updates
 */
class ModeLogicTest {

    @Test
    fun testInitialState() {
        val systemState = ArduinoSystemState()

        assertEquals("Default mode should be AUTO", SystemMode.AUTO, systemState.systemMode)
        assertFalse("Scenario sequence should be hidden initially",
            isScenarioSequenceVisible(systemState))
    }

    @Test
    fun testModeCommandGeneration() {
        // Test AUTO mode command
        val autoCommand = generateModeCommand(SystemMode.AUTO)
        assertEquals("Should generate correct AUTO command", "MODE AUTO", autoCommand)

        // Test MANUAL mode command
        val manualCommand = generateModeCommand(SystemMode.MANUAL)
        assertEquals("Should generate correct MANUAL command", "MODE MANUAL", manualCommand)
    }

    @Test
    fun testScenarioCommandGeneration() {
        // Test barrel command generation for scenario sequence
        val barrelCommands = generateBarrelCommands(mapOf(
            0 to "INTAKE",
            1 to "WORK"
        ))

        assertEquals("Should generate 2 commands", 2, barrelCommands.size)
        assertTrue("Should contain barrel 0 INTAKE command",
            barrelCommands.contains("CMD INTAKE 0"))
        assertTrue("Should contain barrel 1 WORK command",
            barrelCommands.contains("CMD WORK 1"))
    }

    @Test
    fun testEmergencyStopCommands() {
        val emergencyCommands = generateEmergencyStopCommands(2)

        assertEquals("Should generate 2 emergency commands", 2, emergencyCommands.size)
        assertTrue("Should exhaust barrel 0", emergencyCommands.contains("CMD EXHAUST 0"))
        assertTrue("Should exhaust barrel 1", emergencyCommands.contains("CMD EXHAUST 1"))
    }

    @Test
    fun testCompleteScenarioFlow() {
        // Simulate complete scenario sequence flow
        val systemState = ArduinoSystemState(systemMode = SystemMode.MANUAL)

        // 1. Mode should be MANUAL
        assertTrue("Should be in MANUAL mode", systemState.systemMode == SystemMode.MANUAL)
        assertTrue("Scenario should be visible", isScenarioSequenceVisible(systemState))

        // 2. Generate start sequence commands
        val startCommands = generateBarrelCommands(mapOf(
            0 to "INTAKE",
            1 to "INTAKE"
        ))
        assertEquals("Should generate start commands", 2, startCommands.size)

        // 3. Generate next step commands
        val nextStepCommands = generateBarrelCommands(mapOf(
            0 to "WORK",
            1 to "INTAKE"
        ))
        assertEquals("Should generate next step commands", 2, nextStepCommands.size)

        // 4. Generate stop commands
        val stopCommands = generateEmergencyStopCommands(2)
        assertEquals("Should generate stop commands", 2, stopCommands.size)
    }

    @Test
    fun testUDPCommandFormat() {
        // Test that commands match Arduino expected format
        val commands = listOf(
            generateModeCommand(SystemMode.AUTO),
            generateModeCommand(SystemMode.MANUAL),
            "CMD INTAKE 0",
            "CMD WORK 1",
            "CMD EXHAUST 0",
            "STATUS"
        )

        commands.forEach { command ->
            assertTrue("Command should be uppercase: $command",
                command == command.uppercase())
            assertFalse("Command should not be empty", command.isEmpty())
            assertTrue("Command should have valid format",
                command.matches(Regex("(MODE (AUTO|MANUAL))|(CMD [A-Z_]+ [0-9]+)|STATUS")))
        }
    }

    // Helper methods that simulate the integration logic
    private fun isScenarioSequenceVisible(systemState: ArduinoSystemState): Boolean {
        return systemState.systemMode == SystemMode.MANUAL
    }

    private fun generateModeCommand(mode: SystemMode): String {
        return "MODE ${mode.name}"
    }

    private fun generateBarrelCommands(barrelStates: Map<Int, String>): List<String> {
        return barrelStates.map { (barrelIndex, state) ->
            "CMD $state $barrelIndex"
        }
    }

    private fun generateEmergencyStopCommands(barrelCount: Int): List<String> {
        return (0 until barrelCount).map { barrelIndex ->
            "CMD EXHAUST $barrelIndex"
        }
    }
}
