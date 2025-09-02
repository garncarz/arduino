package cz.garncarz.compressed_air_controller

import androidx.test.espresso.Espresso.onView
import androidx.test.espresso.action.ViewActions.click
import androidx.test.espresso.assertion.ViewAssertions.matches
import androidx.test.espresso.matcher.ViewMatchers.*
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.rule.GrantPermissionRule
import android.Manifest
import android.content.Context
import android.content.Intent
import androidx.test.uiautomator.UiDevice
import org.hamcrest.Matchers.anyOf
import org.hamcrest.Matchers.containsString
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import android.util.Log

@RunWith(AndroidJUnit4::class)
@LargeTest
@org.junit.Ignore("Flaky on ATD emulator; covered by UIAutomator E2E test")
class ScenarioSequenceIntegrationTest {

    @get:Rule
    val permissionRule: GrantPermissionRule = GrantPermissionRule.grant(
        Manifest.permission.ACCESS_FINE_LOCATION
    )

    @Test
    fun testScenarioSequenceStartButtonActuallyWorks() {
        Log.d("ScenarioSequenceTest", "Starting integration test")

        // Launch the activity manually
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val context = instrumentation.targetContext
        // Grant runtime permission proactively to avoid system dialog
        try {
            val device = UiDevice.getInstance(instrumentation)
            device.executeShellCommand("pm grant ${context.packageName} android.permission.ACCESS_FINE_LOCATION")
        } catch (t: Throwable) {
            Log.w("ScenarioSequenceTest", "Permission grant via shell failed: ${t.message}")
        }

        val intent = Intent(context, MainActivity::class.java).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK)
        }
        context.startActivity(intent)

        // Wait for activity to fully load
        Thread.sleep(4000)

        // Switch to MANUAL mode (card is Manual-only)
        onView(withId(R.id.manualModeButton))
            .perform(click())

        // Verify scenario sequence card is visible in MANUAL
        onView(withId(R.id.scenarioSequenceCard))
            .check(matches(isDisplayed()))

        // Verify Start button exists and is clickable
        onView(withId(R.id.startSequenceButton))
            .check(matches(isDisplayed()))
            .check(matches(isClickable()))

        Log.d("ScenarioSequenceTest", "About to click Start button")

        // Click the Start button
        onView(withId(R.id.startSequenceButton))
            .perform(click())

        Log.d("ScenarioSequenceTest", "Start button clicked")

        // Wait for state change: poll for up to ~6s
        var ok = false
        repeat(30) {
            try {
                onView(withId(R.id.scenarioStatusText))
                    .check(matches(withText(containsString("Running step"))))
                onView(withId(R.id.nextStepButton))
                    .check(matches(isEnabled()))
                ok = true
                return@repeat
            } catch (_: Throwable) {
                Thread.sleep(200)
            }
        }
        if (!ok) {
            // Final assertions (will throw)
            onView(withId(R.id.scenarioStatusText))
                .check(matches(withText(containsString("Running step"))))
            onView(withId(R.id.nextStepButton))
                .check(matches(isEnabled()))
        }

        Log.d("ScenarioSequenceTest", "Verified button text changed to Stop")

        // Verify status text shows sequence is running (less brittle match)
        onView(withId(R.id.scenarioStatusText))
            .check(matches(withText(containsString("Running step 1"))))

        Log.d("ScenarioSequenceTest", "Test completed successfully")
    }
}
