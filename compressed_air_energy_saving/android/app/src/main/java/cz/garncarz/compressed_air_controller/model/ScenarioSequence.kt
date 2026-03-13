package cz.garncarz.compressed_air_controller.model

/**
 * Represents a single step in the scenario sequence with barrel states and explanation
 */
data class SequenceStep(
    val stepNumber: Int,
    val description: String,
    val barrelStates: Map<Int, BarrelState>,
    val explanation: String
)

/**
 * ScenarioSequence simulates optimized 2-barrel compressed air coordination
 *
 * Complete 8-step sequence:
 * Step 1: Both INTAKE (initial pressurization)
 * Step 2: Barrel 0 WORK, Barrel 1 INTAKE
 * Step 3: Barrel 0 WORK, Barrel 1 WAIT_FOR_WORK
 * Step 4: Barrel 0 EXHAUST, Barrel 1 WORK (zero-gap handoff)
 * Step 5: Barrel 0 INTAKE, Barrel 1 WORK (direct transition)
 * Step 6: Barrel 0 WAIT_FOR_WORK, Barrel 1 WORK
 * Step 7: Barrel 0 WORK, Barrel 1 EXHAUST (second handoff)
 * Step 8: Barrel 0 WORK, Barrel 1 INTAKE (cycle continuation)
 * Then loops back to step 2 for continuous operation
 */
class ScenarioSequence(private val barrelCount: Int) {

    private var currentStepIndex = -1
    private var isSequenceRunning = false
    private val sequenceSteps: List<SequenceStep>

    init {
        // Initialize the 8-step continuous 2-barrel sequence
        sequenceSteps = createOptimizedSequence()
    }

    private fun createOptimizedSequence(): List<SequenceStep> {
        if (barrelCount != 2) {
            throw IllegalArgumentException("ScenarioSequence currently supports only 2-barrel systems")
        }

        return listOf(
            SequenceStep(
                stepNumber = 1,
                description = "Initial Pressurization",
                barrelStates = mapOf(
                    0 to BarrelState.INTAKE,
                    1 to BarrelState.INTAKE
                ),
                explanation = "Both barrels begin pressurization cycle to reach working pressure"
            ),
            SequenceStep(
                stepNumber = 2,
                description = "First Barrel Working",
                barrelStates = mapOf(
                    0 to BarrelState.WORK,
                    1 to BarrelState.INTAKE
                ),
                explanation = "Barrel 0 reaches pressure and starts working while Barrel 1 continues pressurizing"
            ),
            SequenceStep(
                stepNumber = 3,
                description = "Preparation Phase",
                barrelStates = mapOf(
                    0 to BarrelState.WORK,
                    1 to BarrelState.WAIT_FOR_WORK
                ),
                explanation = "Barrel 1 reaches pressure and waits for handoff while Barrel 0 continues working"
            ),
            SequenceStep(
                stepNumber = 4,
                description = "Zero-Gap Handoff",
                barrelStates = mapOf(
                    0 to BarrelState.EXHAUST,
                    1 to BarrelState.WORK
                ),
                explanation = "Critical handoff: Barrel 0 exhausts as Barrel 1 takes over working"
            ),
            SequenceStep(
                stepNumber = 5,
                description = "Direct Transition",
                barrelStates = mapOf(
                    0 to BarrelState.INTAKE,
                    1 to BarrelState.WORK
                ),
                explanation = "Barrel 0 directly transitions to intake for next cycle while Barrel 1 continues working"
            ),
            SequenceStep(
                stepNumber = 6,
                description = "Second Preparation",
                barrelStates = mapOf(
                    0 to BarrelState.WAIT_FOR_WORK,
                    1 to BarrelState.WORK
                ),
                explanation = "Barrel 0 reaches pressure and waits while Barrel 1 continues working"
            ),
            SequenceStep(
                stepNumber = 7,
                description = "Second Handoff",
                barrelStates = mapOf(
                    0 to BarrelState.WORK,
                    1 to BarrelState.EXHAUST
                ),
                explanation = "Second handoff: Barrel 1 exhausts as Barrel 0 takes over working"
            ),
            SequenceStep(
                stepNumber = 8,
                description = "Cycle Continuation",
                barrelStates = mapOf(
                    0 to BarrelState.WORK,
                    1 to BarrelState.INTAKE
                ),
                explanation = "Barrel 1 transitions to intake, completing one full cycle. Loops to step 2."
            )
        )
    }

    /**
     * Start the scenario sequence from the beginning
     * @return true if sequence started successfully, false if already running
     */
    fun startSequence(): Boolean {
        if (isSequenceRunning) {
            return false
        }

        isSequenceRunning = true
        currentStepIndex = -1  // Will be incremented to 0 on first nextStep() call
        return true
    }

    /**
     * Advance to the next step in the sequence
     * @return the next SequenceStep, or null if sequence is not running
     */
    fun nextStep(): SequenceStep? {
        if (!isSequenceRunning) {
            return null
        }

        currentStepIndex += 1

        // After step 8, loop back to step 2 (skip initial pressurization)
        if (currentStepIndex >= sequenceSteps.size) {
            currentStepIndex = 1  // Loop to step 2
        }

        return sequenceSteps[currentStepIndex]
    }

    /**
     * Emergency stop - returns EXHAUST for initial safe depressurization
     * (EXIT will be used in stage 2 of the emergency procedure after depressurization is complete)
     * @return Map of barrel states for emergency stop stage 1 (all EXHAUST)
     */
    fun stopSequence(): Map<Int, BarrelState> {
        isSequenceRunning = false
        currentStepIndex = -1

        // Return emergency state: all barrels EXHAUST for safe depressurization
        return (0 until barrelCount).associateWith { BarrelState.EXHAUST }
    }

    /**
     * Get the current sequence step without advancing
     * @return current SequenceStep or null if not running or at start
     */
    fun getCurrentStep(): SequenceStep? {
        if (!isSequenceRunning || currentStepIndex == -1) {
            return null
        }
        return sequenceSteps[currentStepIndex]
    }

    /**
     * Check if sequence is currently running
     * @return true if sequence is active
     */
    fun isRunning(): Boolean = isSequenceRunning

    /**
     * Get descriptive information about this sequence
     * @return String describing the sequence type and barrel count
     */
    fun getSequenceInfo(): String = "$barrelCount-barrel optimized coordination sequence"

    /**
     * Get total number of steps in the sequence
     * @return total step count
     */
    fun getTotalSteps(): Int = sequenceSteps.size

    /**
     * Get step progress as a formatted string
     * @return formatted progress string like "Step 3/8" or "Ready" if not running
     */
    fun getProgressString(): String {
        if (!isSequenceRunning || currentStepIndex == -1) {
            return "Ready"
        }
        return "Step ${currentStepIndex + 1}/${sequenceSteps.size}"
    }
}
