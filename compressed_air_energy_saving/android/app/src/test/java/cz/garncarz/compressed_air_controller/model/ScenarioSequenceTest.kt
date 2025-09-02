package cz.garncarz.compressed_air_controller.model

import cz.garncarz.compressed_air_controller.model.BarrelState
import cz.garncarz.compressed_air_controller.model.ScenarioSequence
import org.junit.Test
import org.junit.Assert.*

/**
 * Fast unit tests for ScenarioSequence 2-barrel algorithm
 * Tests the complete 8-step optimization sequence
 */
class ScenarioSequenceTest {

    @Test
    fun testScenarioSequenceInitialization() {
        val sequence = ScenarioSequence(2)

        assertFalse("Sequence should not be running initially", sequence.isRunning())
        assertEquals("Should have 8 total steps", 8, sequence.getTotalSteps())
        assertEquals("Should show ready status", "Ready", sequence.getProgressString())
        assertEquals("Should describe 2-barrel sequence", "2-barrel optimized coordination sequence",
            sequence.getSequenceInfo())
        assertNull("Should not have current step initially", sequence.getCurrentStep())
    }

    @Test
    fun testSequenceStartAndFirstStep() {
        val sequence = ScenarioSequence(2)

        assertTrue("Should successfully start sequence", sequence.startSequence())
        assertTrue("Sequence should be running after start", sequence.isRunning())
        assertFalse("Should not be able to start again", sequence.startSequence())

        val firstStep = sequence.nextStep()
        assertNotNull("Should get first step", firstStep)
        assertEquals("First step should be step 1", 1, firstStep!!.stepNumber)
        assertEquals("First step should be Initial Pressurization", "Initial Pressurization", firstStep.description)

        // Verify both barrels start with INTAKE
        assertEquals("Barrel 0 should be INTAKE", BarrelState.INTAKE, firstStep.barrelStates[0])
        assertEquals("Barrel 1 should be INTAKE", BarrelState.INTAKE, firstStep.barrelStates[1])

        assertEquals("Should show step 1 progress", "Step 1/8", sequence.getProgressString())
    }

    @Test
    fun testComplete8StepSequence() {
        val sequence = ScenarioSequence(2)
        sequence.startSequence()

        // Expected sequence of barrel states for 8 steps
        val expectedSequence = listOf(
            // Step 1: Both INTAKE
            mapOf(0 to BarrelState.INTAKE, 1 to BarrelState.INTAKE),
            // Step 2: Barrel 0 WORK, Barrel 1 INTAKE
            mapOf(0 to BarrelState.WORK, 1 to BarrelState.INTAKE),
            // Step 3: Barrel 0 WORK, Barrel 1 WAIT_FOR_WORK
            mapOf(0 to BarrelState.WORK, 1 to BarrelState.WAIT_FOR_WORK),
            // Step 4: Barrel 0 EXHAUST, Barrel 1 WORK (zero-gap handoff)
            mapOf(0 to BarrelState.EXHAUST, 1 to BarrelState.WORK),
            // Step 5: Barrel 0 INTAKE, Barrel 1 WORK (direct transition)
            mapOf(0 to BarrelState.INTAKE, 1 to BarrelState.WORK),
            // Step 6: Barrel 0 WAIT_FOR_WORK, Barrel 1 WORK
            mapOf(0 to BarrelState.WAIT_FOR_WORK, 1 to BarrelState.WORK),
            // Step 7: Barrel 0 WORK, Barrel 1 EXHAUST (second handoff)
            mapOf(0 to BarrelState.WORK, 1 to BarrelState.EXHAUST),
            // Step 8: Barrel 0 WORK, Barrel 1 INTAKE (cycle continuation)
            mapOf(0 to BarrelState.WORK, 1 to BarrelState.INTAKE)
        )

        // Test all 8 steps
        for (expectedStepIndex in expectedSequence.indices) {
            val step = sequence.nextStep()
            assertNotNull("Should get step ${expectedStepIndex + 1}", step)

            val expectedStates = expectedSequence[expectedStepIndex]
            assertEquals("Step ${expectedStepIndex + 1} should have correct step number",
                expectedStepIndex + 1, step!!.stepNumber)

            expectedStates.forEach { (barrelIndex, expectedState) ->
                assertEquals("Step ${expectedStepIndex + 1}: Barrel $barrelIndex should be $expectedState",
                    expectedState, step.barrelStates[barrelIndex])
            }

            assertEquals("Should show correct progress",
                "Step ${expectedStepIndex + 1}/8", sequence.getProgressString())
        }

    // After step 8, sequence loops endlessly over steps 2..8 (no repeated step 1)
    val nextStep = sequence.nextStep()
    assertEquals("Should loop to step 2 (skip step 1 after initialization)", 2, nextStep!!.stepNumber)
    }

    @Test
    fun testZeroGapHandoffSteps() {
        val sequence = ScenarioSequence(2)
        sequence.startSequence()

        // Advance to step 4 (zero-gap handoff)
        repeat(4) { sequence.nextStep() }

        val step4 = sequence.getCurrentStep()
        assertNotNull("Should have step 4", step4)
        assertEquals("Should be step 4", 4, step4!!.stepNumber)
        assertEquals("Should be zero-gap handoff", "Zero-Gap Handoff", step4.description)

        // Verify critical handoff: Barrel 0 EXHAUST, Barrel 1 WORK
        assertEquals("Barrel 0 should be EXHAUST during handoff",
            BarrelState.EXHAUST, step4.barrelStates[0])
        assertEquals("Barrel 1 should be WORK during handoff",
            BarrelState.WORK, step4.barrelStates[1])
    }

    @Test
    fun testDirectTransitionOptimization() {
        val sequence = ScenarioSequence(2)
        sequence.startSequence()

        // Advance to step 5 (direct transition)
        repeat(5) { sequence.nextStep() }

        val step5 = sequence.getCurrentStep()
        assertNotNull("Should have step 5", step5)
        assertEquals("Should be step 5", 5, step5!!.stepNumber)
        assertEquals("Should be direct transition", "Direct Transition", step5.description)

        // Verify optimization: Barrel 0 goes directly to INTAKE (skips wait)
        assertEquals("Barrel 0 should skip wait and go to INTAKE",
            BarrelState.INTAKE, step5.barrelStates[0])
        assertEquals("Barrel 1 should continue WORK",
            BarrelState.WORK, step5.barrelStates[1])
    }

    @Test
    fun testEmergencyStop() {
        val sequence = ScenarioSequence(2)
        sequence.startSequence()

        // Advance a few steps
        sequence.nextStep()
        sequence.nextStep()
        assertTrue("Should be running", sequence.isRunning())

        // Emergency stop
        val emergencyStates = sequence.stopSequence()

        assertFalse("Should not be running after stop", sequence.isRunning())
        assertEquals("Should show ready status", "Ready", sequence.getProgressString())
        assertNull("Should not have current step after stop", sequence.getCurrentStep())

        // Verify all barrels set to EXHAUST
        assertEquals("Should have 2 barrels in emergency state", 2, emergencyStates.size)
        assertEquals("Barrel 0 should be EXHAUST", BarrelState.EXHAUST, emergencyStates[0])
        assertEquals("Barrel 1 should be EXHAUST", BarrelState.EXHAUST, emergencyStates[1])
    }

    @Test
    fun testSequenceRestartAfterStop() {
        val sequence = ScenarioSequence(2)

        // Start, advance, then stop
        sequence.startSequence()
        sequence.nextStep()
        sequence.nextStep()
        sequence.stopSequence()

        // Should be able to restart
        assertTrue("Should be able to restart after stop", sequence.startSequence())
        assertTrue("Should be running after restart", sequence.isRunning())

        val firstStep = sequence.nextStep()
        assertEquals("Should restart at step 1", 1, firstStep!!.stepNumber)
    }

    @Test
    fun testInvalidBarrelCount() {
        try {
            ScenarioSequence(1)  // Only 1 barrel
            fail("Should throw exception for invalid barrel count")
        } catch (e: IllegalArgumentException) {
            assertTrue("Should mention 2-barrel requirement",
                e.message!!.contains("2-barrel"))
        }

        try {
            ScenarioSequence(3)  // More than 2 barrels
            fail("Should throw exception for unsupported barrel count")
        } catch (e: IllegalArgumentException) {
            assertTrue("Should mention 2-barrel requirement",
                e.message!!.contains("2-barrel"))
        }
    }

    @Test
    fun testNextStepWithoutStart() {
        val sequence = ScenarioSequence(2)

        assertNull("Should return null when not started", sequence.nextStep())
        assertFalse("Should not be running", sequence.isRunning())
    }

    @Test
    fun testSequenceDescriptionsAreInformative() {
        val sequence = ScenarioSequence(2)
        sequence.startSequence()

        val stepDescriptions = (1..8).map {
            sequence.nextStep()!!.description
        }

        // Verify key descriptions
        assertTrue("Should have Initial Pressurization",
            stepDescriptions.contains("Initial Pressurization"))
        assertTrue("Should have Zero-Gap Handoff",
            stepDescriptions.contains("Zero-Gap Handoff"))
        assertTrue("Should have Direct Transition",
            stepDescriptions.contains("Direct Transition"))
        assertTrue("Should have Cycle Continuation",
            stepDescriptions.contains("Cycle Continuation"))
    }
}
