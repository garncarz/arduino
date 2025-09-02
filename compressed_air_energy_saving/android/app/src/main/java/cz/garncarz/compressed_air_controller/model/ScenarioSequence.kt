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
 * 8-Step Optimization Algorithm:
 * Step 1: Both INTAKE (initial pressurization)
 * Step 2: Barrel 0 WORK, Barrel 1 INTAKE (first barrel ready)
 * Step 3: Barrel 0 WORK, Barrel 1 WAIT_FOR_WORK (second barrel ready, waiting)
 * Step 4: Barrel 0 EXHAUST, Barrel 1 WORK (zero-gap handoff)
 * Step 5: Barrel 0 INTAKE, Barrel 1 WORK (direct transition optimization)
 * Step 6: Barrel 0 WAIT_FOR_WORK, Barrel 1 WORK (first barrel ready again)
 * Step 7: Barrel 0 WORK, Barrel 1 EXHAUST (second handoff)
 * Step 8: Barrel 0 WORK, Barrel 1 INTAKE (cycle continues)
 */
class ScenarioSequence(private val barrelCount: Int) {

    private var currentStepIndex = -1
    private var isSequenceRunning = false
    private val sequenceSteps: List<SequenceStep>
    private var initialized = false

    init {
        // Initialize the 8-step 2-barrel optimization sequence
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
                explanation = "Both barrels begin simultaneous pressurization for system startup"
            ),
            SequenceStep(
                stepNumber = 2,
                description = "First Barrel Ready",
                barrelStates = mapOf(
                    0 to BarrelState.WORK,
                    1 to BarrelState.INTAKE
                ),
                explanation = "Barrel 0 reaches pressure and starts working while Barrel 1 continues pressurizing"
            ),
            SequenceStep(
                stepNumber = 3,
                description = "Second Barrel Ready",
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
                explanation = "Critical transition: Barrel 0 exhausts precisely as Barrel 1 takes over working"
            ),
            SequenceStep(
                stepNumber = 5,
                description = "Direct Transition",
                barrelStates = mapOf(
                    0 to BarrelState.INTAKE,
                    1 to BarrelState.WORK
                ),
                explanation = "Optimization: Barrel 0 skips wait state and directly starts intake while Barrel 1 works"
            ),
            SequenceStep(
                stepNumber = 6,
                description = "First Barrel Ready Again",
                barrelStates = mapOf(
                    0 to BarrelState.WAIT_FOR_WORK,
                    1 to BarrelState.WORK
                ),
                explanation = "Barrel 0 reaches pressure again and waits for next handoff"
            ),
            SequenceStep(
                stepNumber = 7,
                description = "Second Handoff",
                barrelStates = mapOf(
                    0 to BarrelState.WORK,
                    1 to BarrelState.EXHAUST
                ),
                explanation = "Roles reverse: Barrel 0 takes over working while Barrel 1 exhausts"
            ),
            SequenceStep(
                stepNumber = 8,
                description = "Cycle Continuation",
                barrelStates = mapOf(
                    0 to BarrelState.WORK,
                    1 to BarrelState.INTAKE
                ),
                explanation = "Barrel 1 begins next cycle intake while Barrel 0 works. Sequence loops back to step 2."
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
        // First advancement returns step 1 (both INTAKE) once.
        if (!initialized) {
            initialized = true
            currentStepIndex = 0
            return sequenceSteps[currentStepIndex]
        }
        // After initialization, loop endlessly over steps 2..8, never returning step 1 again.
        if (currentStepIndex < 1) {
            currentStepIndex = 1
        } else if (currentStepIndex >= sequenceSteps.lastIndex) {
            currentStepIndex = 1  // wrap back to step 2
        } else {
            currentStepIndex += 1
        }
        return sequenceSteps[currentStepIndex]
    }

    /**
     * Emergency stop - immediately set all barrels to EXHAUST
     * @return Map of barrel states for emergency stop (all EXHAUST)
     */
    fun stopSequence(): Map<Int, BarrelState> {
        isSequenceRunning = false
        currentStepIndex = -1
    initialized = false

        // Return emergency state: all barrels EXHAUST
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
