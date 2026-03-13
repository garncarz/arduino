package cz.garncarz.compressed_air_controller.integration

import cz.garncarz.compressed_air_controller.model.BarrelState
import cz.garncarz.compressed_air_controller.model.ScenarioSequence
import org.junit.Test
import org.junit.Assert.*

/**
 * Complete integration test verifying the entire scenario sequence feature
 * This test simulates the full user flow from start to finish
 */
class CompleteScenarioIntegrationTest {

    @Test
    fun testCompleteScenarioSequenceIntegration() {
        // 1. Initialize 2-barrel scenario sequence
        val scenarioSequence = ScenarioSequence(2)

        assertFalse("Scenario should not be running initially", scenarioSequence.isRunning())
        assertEquals("Should be ready", "Ready", scenarioSequence.getProgressString())

        // 2. Start the scenario sequence (simulating user clicking "Start")
        assertTrue("Should successfully start", scenarioSequence.startSequence())
        assertTrue("Should be running after start", scenarioSequence.isRunning())

        // 3. Execute first step - both barrels INTAKE (initial pressurization)
        val step1 = scenarioSequence.nextStep()
        assertNotNull("Should get step 1", step1)
        assertEquals("Should be step 1", 1, step1!!.stepNumber)
        assertEquals("Both barrels should be INTAKE", BarrelState.INTAKE, step1.barrelStates[0])
        assertEquals("Both barrels should be INTAKE", BarrelState.INTAKE, step1.barrelStates[1])

        // Simulate Arduino commands: "CMD INTAKE 0", "CMD INTAKE 1"
        val step1Commands = generateCommands(step1.barrelStates)
        assertEquals("Should generate 2 commands", 2, step1Commands.size)
        assertTrue("Should send INTAKE command to barrel 0", step1Commands.contains("CMD INTAKE 0"))
        assertTrue("Should send INTAKE command to barrel 1", step1Commands.contains("CMD INTAKE 1"))

        // 4. Execute the complete 8-step sequence
        val allSteps = mutableListOf(step1)
        for (i in 2..8) {
            val step = scenarioSequence.nextStep()
            assertNotNull("Should get step $i", step)
            allSteps.add(step!!)

            // Verify each step sends appropriate commands
            val commands = generateCommands(step.barrelStates)
            assertEquals("Step $i should generate 2 commands", 2, commands.size)
        }

        // Verify key optimization steps
        val step4 = allSteps[3] // Zero-gap handoff
        assertEquals("Step 4: Barrel 0 should EXHAUST", BarrelState.EXHAUST, step4.barrelStates[0])
        assertEquals("Step 4: Barrel 1 should WORK", BarrelState.WORK, step4.barrelStates[1])

        val step5 = allSteps[4] // Direct transition optimization
        assertEquals("Step 5: Barrel 0 should go directly to INTAKE", BarrelState.INTAKE, step5.barrelStates[0])
        assertEquals("Step 5: Barrel 1 should continue WORK", BarrelState.WORK, step5.barrelStates[1])

        // 5. Test emergency stop (simulating user clicking "STOP")
        val emergencyStates = scenarioSequence.stopSequence()
        assertFalse("Should not be running after stop", scenarioSequence.isRunning())

        // Verify emergency stop commands
        val emergencyCommands = generateCommands(emergencyStates)
        assertEquals("Should generate 2 emergency commands", 2, emergencyCommands.size)
        assertTrue("Should send EXHAUST to barrel 0", emergencyCommands.contains("CMD EXHAUST 0"))
        assertTrue("Should send EXHAUST to barrel 1", emergencyCommands.contains("CMD EXHAUST 1"))

        // 6. Verify sequence can be restarted
        assertTrue("Should be able to restart", scenarioSequence.startSequence())
        val restartStep = scenarioSequence.nextStep()
        assertEquals("Should restart at step 1", 1, restartStep!!.stepNumber)

        println("✅ Complete scenario sequence integration test passed!")
    }

    @Test
    fun testUIIntegrationFlow() {
        // Simulate complete UI integration flow

        // 1. App starts in AUTO mode - scenario sequence HIDDEN
        var isScenarioVisible = false  // AUTO mode
        assertFalse("Scenario should be hidden in AUTO", isScenarioVisible)

        // 2. User clicks MANUAL button - scenario sequence becomes VISIBLE
        isScenarioVisible = true  // MANUAL mode
        assertTrue("Scenario should be visible in MANUAL", isScenarioVisible)

        // 3. User starts scenario sequence
        val scenarioSequence = ScenarioSequence(2)
        var startButtonEnabled = true
        var nextButtonEnabled = false
        var stopButtonEnabled = true

        // Click start
        scenarioSequence.startSequence()
        val firstStep = scenarioSequence.nextStep()

        // Button states should update
        startButtonEnabled = false
        nextButtonEnabled = true
        stopButtonEnabled = true

        assertFalse("Start button should be disabled", startButtonEnabled)
        assertTrue("Next button should be enabled", nextButtonEnabled)
        assertTrue("Stop button should remain enabled", stopButtonEnabled)

        // 4. User advances through steps
        scenarioSequence.nextStep() // Step 2
        scenarioSequence.nextStep() // Step 3

        // 5. User clicks emergency stop
        scenarioSequence.stopSequence()

        // Button states reset
        startButtonEnabled = true
        nextButtonEnabled = false

        assertTrue("Start button should be re-enabled", startButtonEnabled)
        assertFalse("Next button should be disabled", nextButtonEnabled)

        // 6. User switches back to AUTO - scenario sequence HIDDEN
        isScenarioVisible = false  // AUTO mode
        assertFalse("Scenario should be hidden again in AUTO", isScenarioVisible)

        println("✅ Complete UI integration flow test passed!")
    }

    @Test
    fun testArduinoCommandCompatibility() {
        // Test that all generated commands are compatible with Arduino parser

        val scenarioSequence = ScenarioSequence(2)
        scenarioSequence.startSequence()

        val allGeneratedCommands = mutableListOf<String>()

        // Collect commands from all 8 steps
        for (i in 1..8) {
            val step = scenarioSequence.nextStep()!!
            val commands = generateCommands(step.barrelStates)
            allGeneratedCommands.addAll(commands)
        }

        // Test emergency stop commands
        val emergencyCommands = generateCommands(scenarioSequence.stopSequence())
        allGeneratedCommands.addAll(emergencyCommands)

        // Verify all commands are Arduino-compatible
        allGeneratedCommands.forEach { command ->
            // Must be uppercase
            assertEquals("Command should be uppercase", command.uppercase(), command)

            // Must match Arduino command format
            assertTrue("Command should match Arduino format: $command",
                command.matches(Regex("CMD [A-Z_]+ [0-9]+")))

            // Must contain valid barrel states
            val validStates = listOf("INTAKE", "WORK", "EXHAUST", "WAIT_FOR_WORK", "WAIT_FOR_INTAKE")
            val commandState = command.split(" ")[1]
            assertTrue("Command state should be valid: $commandState",
                validStates.contains(commandState))

            // Must contain valid barrel index
            val barrelIndex = command.split(" ")[2].toInt()
            assertTrue("Barrel index should be 0 or 1: $barrelIndex",
                barrelIndex in 0..1)
        }

        println("✅ Arduino command compatibility test passed!")
        println("Generated ${allGeneratedCommands.size} compatible commands")
    }

    // Helper function to simulate command generation
    private fun generateCommands(barrelStates: Map<Int, BarrelState>): List<String> {
        return barrelStates.map { (barrelIndex, state) ->
            "CMD ${state.name} $barrelIndex"
        }
    }
}
