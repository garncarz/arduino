package cz.garncarz.compressed_air_controller

import androidx.test.ext.junit.rules.ActivityScenarioRule
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.espresso.Espresso.onView
import androidx.test.espresso.action.ViewActions.click
import androidx.test.espresso.assertion.ViewAssertions.matches
import androidx.test.espresso.matcher.ViewMatchers.*
import androidx.test.espresso.IdlingPolicies
import org.junit.Before
import org.junit.Ignore
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.hamcrest.Matchers.not
import org.hamcrest.Matchers.containsString
import java.util.concurrent.TimeUnit

/**
 * Fast UI tests with aggressive timeouts (<5 seconds total)
 * Tests actual UI visibility behavior on device
 */
@RunWith(AndroidJUnit4::class)
@Ignore("Disabled on emulator: use UIAutomator E2E test")
class RealUITest {

    @get:Rule
    val activityRule = ActivityScenarioRule(MainActivity::class.java)

    @Before
    fun setup() {
        // Set aggressive timeouts for fast test execution
        IdlingPolicies.setMasterPolicyTimeout(3, TimeUnit.SECONDS)
        IdlingPolicies.setIdlingResourceTimeout(2, TimeUnit.SECONDS)
    }

    @Test
    fun testScenarioSequenceAppearsInManualMode() {
        // Verify scenario card hidden initially (app starts in AUTO mode typically)
        onView(withId(R.id.scenarioSequenceCard))
            .check(matches(not(isDisplayed())))

        // Click MANUAL button
        onView(withText("MANUAL Mode"))
            .perform(click())

        // Scenario sequence MUST appear - this is the critical test
        onView(withId(R.id.scenarioSequenceCard))
            .check(matches(isDisplayed()))

        // Verify scenario sequence elements are visible
        onView(withId(R.id.scenarioStatusText))
            .check(matches(isDisplayed()))
        onView(withId(R.id.startSequenceButton))
            .check(matches(isDisplayed()))
        onView(withId(R.id.nextStepButton))
            .check(matches(isDisplayed()))
        onView(withId(R.id.stopSequenceButton))
            .check(matches(isDisplayed()))
    }

    @Test
    fun testScenarioSequenceHidesInAutoMode() {
        // First switch to MANUAL to show scenario
        onView(withText("MANUAL Mode"))
            .perform(click())

        // Verify it's visible
        onView(withId(R.id.scenarioSequenceCard))
            .check(matches(isDisplayed()))

        // Switch back to AUTO
        onView(withText("AUTO Mode"))
            .perform(click())

        // Scenario sequence MUST be hidden
        onView(withId(R.id.scenarioSequenceCard))
            .check(matches(not(isDisplayed())))
    }

    @Test
    fun testScenarioSequenceButtonStates() {
        // Switch to MANUAL mode
        onView(withText("MANUAL Mode"))
            .perform(click())

        // Initial button states
        onView(withId(R.id.startSequenceButton))
            .check(matches(isEnabled()))
        onView(withId(R.id.nextStepButton))
            .check(matches(not(isEnabled())))
        onView(withId(R.id.stopSequenceButton))
            .check(matches(isEnabled()))
    }

    @Test
    fun testScenarioSequenceStartFlow() {
        // Switch to MANUAL mode
        onView(withText("MANUAL Mode"))
            .perform(click())

        // Verify initial status
        onView(withId(R.id.scenarioStatusText))
            .check(matches(withText(containsString("Ready to start"))))

        // Click start button
        onView(withId(R.id.startSequenceButton))
            .perform(click())

        // Button states after start: Start toggles to pause (enabled), Next enabled
        onView(withId(R.id.startSequenceButton))
            .check(matches(isEnabled()))
        onView(withId(R.id.nextStepButton))
            .check(matches(isEnabled()))
    }

    @Test
    fun testScenarioSequenceStopFlow() {
        // Switch to MANUAL mode and start sequence
        onView(withText("MANUAL Mode"))
            .perform(click())
        onView(withId(R.id.startSequenceButton))
            .perform(click())

        // Click stop button
        onView(withId(R.id.stopSequenceButton))
            .perform(click())

        // Should return to ready state
        onView(withId(R.id.startSequenceButton))
            .check(matches(isEnabled()))
        onView(withId(R.id.nextStepButton))
            .check(matches(not(isEnabled())))
        onView(withId(R.id.scenarioStatusText))
            .check(matches(withText(containsString("Ready to start"))))
    }

    @Test
    fun testScenarioSequenceVisibility() {
        // AUTO: scenario hidden
        onView(withText("AUTO Mode"))
            .perform(click())
        onView(withId(R.id.scenarioSequenceCard))
            .check(matches(not(isDisplayed())))

        // MANUAL: scenario visible
        onView(withText("MANUAL Mode"))
            .perform(click())
        onView(withId(R.id.scenarioSequenceCard))
            .check(matches(isDisplayed()))
    }

    @Test
    fun testScenarioSequenceContainsRequiredText() {
        // Switch to MANUAL mode
        onView(withText("MANUAL Mode"))
            .perform(click())

        // Verify scenario sequence card is visible and action buttons exist
        onView(withId(R.id.scenarioSequenceCard))
            .check(matches(isDisplayed()))
        onView(withId(R.id.scenarioStatusText))
            .check(matches(isDisplayed()))
        onView(withId(R.id.startSequenceButton))
            .check(matches(isDisplayed()))
        onView(withId(R.id.nextStepButton))
            .check(matches(isDisplayed()))
        onView(withId(R.id.stopSequenceButton))
            .check(matches(isDisplayed()))
    }

    @Test
    fun testRapidModeToggling() {
        // Test rapid switching between modes
        repeat(3) {
            // Switch to MANUAL
            onView(withText("MANUAL Mode"))
                .perform(click())
            onView(withId(R.id.scenarioSequenceCard))
                .check(matches(isDisplayed()))

            // Switch to AUTO
            onView(withText("AUTO Mode"))
                .perform(click())
            onView(withId(R.id.scenarioSequenceCard))
                .check(matches(not(isDisplayed())))
        }
    }

    // Removed testSystemModeTextUpdates since mode label was removed
}
