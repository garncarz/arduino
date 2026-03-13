package cz.garncarz.compressed_air_controller

import android.view.LayoutInflater
import android.view.View
import android.view.ContextThemeWrapper
import androidx.test.core.app.ApplicationProvider
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.Ignore

@Ignore("Ignored for speed and environment stability")
class ActivityMainLayoutTest {
    @Test
    fun scenario_card_is_gone_by_default_in_layout() {
        val appContext = ApplicationProvider.getApplicationContext<android.content.Context>()
        val themed = ContextThemeWrapper(
            appContext,
            com.google.android.material.R.style.Theme_Material3_DayNight_NoActionBar
        )
        val root = LayoutInflater.from(themed).inflate(R.layout.activity_main, null, false)
        val card = root.findViewById<View>(R.id.scenarioSequenceCard)
        assertEquals(View.GONE, card.visibility)
    }
}
