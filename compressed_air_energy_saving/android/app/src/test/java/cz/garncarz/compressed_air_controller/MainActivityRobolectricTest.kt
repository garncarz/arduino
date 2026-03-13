package cz.garncarz.compressed_air_controller

import android.view.View
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.Ignore
import org.junit.runner.RunWith
import org.robolectric.Robolectric
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(manifest = Config.NONE)
@Ignore("Ignored to keep fast unit suite; UI is covered by device tests")
class MainActivityRobolectricTest {

    @Test
    fun scenario_card_hidden_by_default_and_shown_in_manual() {
        val controller = Robolectric.buildActivity(MainActivity::class.java).setup()
        val activity = controller.get()

        val card = activity.findViewById<View>(R.id.scenarioSequenceCard)
        // Hidden by default (AUTO)
        assertEquals(View.GONE, card.visibility)

        // Click MANUAL -> visible
        activity.findViewById<View>(R.id.manualModeButton).performClick()
        assertEquals(View.VISIBLE, card.visibility)

        // Click AUTO -> hidden again
        activity.findViewById<View>(R.id.autoModeButton).performClick()
        assertEquals(View.GONE, card.visibility)
    }
}
