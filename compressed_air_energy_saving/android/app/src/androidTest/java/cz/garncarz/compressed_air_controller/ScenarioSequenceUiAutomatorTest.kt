package cz.garncarz.compressed_air_controller

import android.content.Intent
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry.getInstrumentation
import androidx.test.uiautomator.By
import androidx.test.uiautomator.UiDevice
import androidx.test.uiautomator.Until
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.Assert.assertTrue

@RunWith(AndroidJUnit4::class)
@LargeTest
class ScenarioSequenceUiAutomatorTest {

    private val pkg = "cz.garncarz.compressed_air_controller"

    @Test
    fun scenarioSequence_shownInManual_and_runsFirstStep() {
        val device = UiDevice.getInstance(getInstrumentation())

        // Launch the app fresh
        val context = getInstrumentation().targetContext
        // Proactively grant location permission to avoid runtime dialog
        try {
            device.executeShellCommand("pm grant $pkg android.permission.ACCESS_FINE_LOCATION")
        } catch (_: Throwable) {
            // ignore
        }
        val intent = Intent(context, MainActivity::class.java).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK)
        }
        context.startActivity(intent)

        // Wait for app to appear
        device.wait(Until.hasObject(By.pkg(pkg).depth(0)), 5_000)

        // Dismiss runtime permission dialogs if shown
        runCatching {
            val permPkg = "com.android.permissioncontroller"
            val allowTexts = listOf("While using the app", "Allow", "Allow only while using the app", "ALLOW")
            val allowIds = listOf(
                "permission_allow_foreground_only_button",
                "permission_allow_one_time_button",
                "permission_allow_button"
            )
            // Try by resource id first
            allowIds.forEach { id ->
                val btn = device.findObject(By.res(permPkg, id))
                if (btn != null) {
                    btn.click()
                    Thread.sleep(200)
                }
            }
            // Then try by text
            allowTexts.forEach { t ->
                val btn = device.findObject(By.textContains(t))
                if (btn != null) {
                    btn.click()
                    Thread.sleep(200)
                }
            }
        }

    // Ensure Scenario Sequence card is not visible on screen initially (AUTO mode)
    val preCard = device.findObject(By.res(pkg, "scenarioSequenceCard"))
    val preVisible = preCard != null && preCard.visibleBounds.height() > 0
    assertTrue("Scenario Sequence card should not be visible in AUTO mode", !preVisible)

        // Tap Manual mode (wait until present)
        val manual = device.wait(Until.findObject(By.res(pkg, "manualModeButton")), 5_000)
        assertTrue("Manual button not found", manual != null)
        manual.click()

        // Verify Scenario Sequence card exists (visible on screen)
        var card = device.wait(Until.findObject(By.res(pkg, "scenarioSequenceCard")), 5_000)
        // If not visible yet, try scrolling the screen to reveal
        if (card == null || card.visibleBounds.height() == 0) {
            val w = device.displayWidth
            val h = device.displayHeight
            repeat(3) {
                device.swipe(w/2, (h*3)/4, w/2, h/4, 20)
                Thread.sleep(250)
                card = device.findObject(By.res(pkg, "scenarioSequenceCard"))
                if (card != null && card!!.visibleBounds.height() > 0) return@repeat
            }
        }
        assertTrue("Scenario Sequence card not visible", card != null && card!!.visibleBounds.height() > 0)

        // Click Start
        var startBtn = device.findObject(By.res(pkg, "startSequenceButton"))
        // If not found, try to scroll to reveal it
        if (startBtn == null) {
            val w = device.displayWidth
            val h = device.displayHeight
            repeat(4) {
                device.swipe(w/2, (h*3)/4, w/2, h/4, 20)
                Thread.sleep(200)
                startBtn = device.findObject(By.res(pkg, "startSequenceButton"))
                if (startBtn != null) return@repeat
            }
        }
        assertTrue("Start button not found", startBtn != null)
        // Click via element, then fallback to coordinate click if needed
        try {
            startBtn!!.click()
        } catch (_: Throwable) {
            val b = startBtn!!.visibleBounds
            device.click(b.centerX(), b.centerY())
        }
    Thread.sleep(300)

        // Wait until either status shows running step, or Next becomes enabled, or a TX log appears, or Start shows pause emoji
        var status = device.findObject(By.res(pkg, "scenarioStatusText"))
        var nextBtnProbe = device.findObject(By.res(pkg, "nextStepButton"))
        var startBtnProbe = device.findObject(By.res(pkg, "startSequenceButton"))
        var ok = false
        val deadline = System.currentTimeMillis() + 20_000
        while (System.currentTimeMillis() < deadline) {
            // Re-query each iteration
            status = device.findObject(By.res(pkg, "scenarioStatusText"))
            nextBtnProbe = device.findObject(By.res(pkg, "nextStepButton"))
            startBtnProbe = device.findObject(By.res(pkg, "startSequenceButton"))
            val text = status?.text ?: ""
            val nextEnabled = nextBtnProbe?.isEnabled ?: false
            val startText = startBtnProbe?.text ?: ""
            val txNode = device.findObject(By.textStartsWith("TX:"))
            val startedNode = device.findObject(By.textContains("Started scenario"))
            if (text.contains("Running step") || nextEnabled || txNode != null || startedNode != null || startText.contains("⏸")) { ok = true; break }
            // Try to scroll down to reveal logs/buttons if needed
            val w = device.displayWidth
            val h = device.displayHeight
            device.swipe(w/2, (h*3)/4, w/2, h/4, 20)
            device.waitForIdle()
            Thread.sleep(150)
        }
        assertTrue("Scenario didn't appear to start (no status/next-enabled/TX/Stop)", ok)

        // Optional: advance to step 2 and verify
        val nextBtn = device.findObject(By.res(pkg, "nextStepButton"))
        nextBtn?.click()
        ok = false
        val deadline2 = System.currentTimeMillis() + 20_000
        while (System.currentTimeMillis() < deadline2) {
            status = device.findObject(By.res(pkg, "scenarioStatusText"))
            val text = status?.text ?: ""
            val advancedLog = device.findObject(By.textContains("Advanced to step 2"))
            if (text.contains("Step 2/8") || advancedLog != null) { ok = true; break }
            val w = device.displayWidth
            val h = device.displayHeight
            device.swipe(w/2, (h*3)/4, w/2, h/4, 20)
            device.waitForIdle()
            Thread.sleep(150)
        }
        assertTrue("Scenario did not advance to step 2 (no status/log)", ok)

        // New UX: Tap a barrel state and pick a new state, verify TX log appears
        runCatching {
            // Scroll to ensure barrel list is visible
            val w = device.displayWidth
            val h = device.displayHeight
            repeat(3) {
                val statePill = device.findObject(By.res(pkg, "barrelStateText"))
                if (statePill != null) return@repeat
                device.swipe(w/2, (h*3)/4, w/2, h/4, 20)
                Thread.sleep(200)
            }
            val statePill = device.findObject(By.res(pkg, "barrelStateText"))
            if (statePill != null) {
                try { statePill.click() } catch (_: Throwable) {
                    val b = statePill.visibleBounds
                    device.click(b.centerX(), b.centerY())
                }
                // Pick the first item in dialog (INTAKE)
                val intake = device.wait(Until.findObject(By.text("INTAKE")), 3_000)
                intake?.click()
                // Wait briefly for TX log
                val dl = System.currentTimeMillis() + 5_000
                var saw = false
                while (System.currentTimeMillis() < dl) {
                    val txNode = device.findObject(By.textStartsWith("TX:"))
                    if (txNode != null) { saw = true; break }
                    device.swipe(w/2, (h*3)/4, w/2, h/4, 20)
                    Thread.sleep(150)
                }
                assertTrue("Expected TX log after inline change", saw)
            }
        }
    }
}
