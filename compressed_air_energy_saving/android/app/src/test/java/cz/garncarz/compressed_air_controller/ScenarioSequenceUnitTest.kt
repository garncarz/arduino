package cz.garncarz.compressed_air_controller

import cz.garncarz.compressed_air_controller.model.ScenarioSequence
import cz.garncarz.compressed_air_controller.model.BarrelState
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class ScenarioSequenceUnitTest {

    @Test
    fun start_and_first_two_steps_are_correct() {
        val seq = ScenarioSequence(2)
        assertTrue(seq.startSequence())

        val s1 = seq.nextStep()!!
        assertEquals(1, s1.stepNumber)
        assertEquals(BarrelState.INTAKE, s1.barrelStates[0])
        assertEquals(BarrelState.INTAKE, s1.barrelStates[1])
        assertTrue(seq.isRunning())
        assertEquals("Step 1/8", seq.getProgressString())

        val s2 = seq.nextStep()!!
        assertEquals(2, s2.stepNumber)
        assertEquals(BarrelState.WORK, s2.barrelStates[0])
        assertEquals(BarrelState.INTAKE, s2.barrelStates[1])
        assertEquals("Step 2/8", seq.getProgressString())
    }

    @Test
    fun stop_sets_all_to_exhaust_and_resets() {
        val seq = ScenarioSequence(2)
        seq.startSequence()
        seq.nextStep()
        val emergency = seq.stopSequence()
        assertEquals(BarrelState.EXHAUST, emergency[0])
        assertEquals(BarrelState.EXHAUST, emergency[1])
        assertEquals("Ready", seq.getProgressString())
    }
}
