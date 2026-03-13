package cz.garncarz.compressed_air_controller.util

import cz.garncarz.compressed_air_controller.model.BarrelState
import org.junit.Test
import org.junit.Assert.*

/**
 * Tests for BarrelStateColors utility class
 * Ensures consistent color mapping across the application
 */
class BarrelStateColorsTest {

    @Test
    fun `all barrel states have defined color resources`() {
        // Test that all barrel states return a valid resource ID
        val states = BarrelState.values()
        
        states.forEach { state ->
            val resourceId = BarrelStateColors.getColorResourceForState(state)
            assertTrue("Resource ID for state $state should be valid", resourceId > 0)
        }
    }

    @Test
    fun `specific color resource assignments match expected Android colors`() {
        // Test specific resource assignments based on safety requirements
        assertEquals(
            "INTAKE should use red color (dangerous)", 
            android.R.color.holo_red_dark,
            BarrelStateColors.getColorResourceForState(BarrelState.INTAKE)
        )
        
        assertEquals(
            "WORK should use orange color", 
            android.R.color.holo_orange_dark,
            BarrelStateColors.getColorResourceForState(BarrelState.WORK)
        )
        
        assertEquals(
            "EXHAUST should use blue color", 
            android.R.color.holo_blue_dark,
            BarrelStateColors.getColorResourceForState(BarrelState.EXHAUST)
        )
        
        assertEquals(
            "WAIT_FOR_INTAKE should use green color", 
            android.R.color.holo_green_dark,
            BarrelStateColors.getColorResourceForState(BarrelState.WAIT_FOR_INTAKE)
        )
        
        assertEquals(
            "WAIT_FOR_WORK should use purple color (pressurized)", 
            android.R.color.holo_purple,
            BarrelStateColors.getColorResourceForState(BarrelState.WAIT_FOR_WORK)
        )
    }

    @Test
    fun `all states have unique color resources`() {
        val states = BarrelState.values()
        val colorResources = states.map { BarrelStateColors.getColorResourceForState(it) }
        
        // Verify all color resources are unique
        assertEquals(
            "All states should have unique color resources",
            states.size,
            colorResources.toSet().size
        )
    }

    @Test
    fun `color safety mapping follows requirements`() {
        // INTAKE is dangerous (pressure building) - should be red
        val intakeResource = BarrelStateColors.getColorResourceForState(BarrelState.INTAKE)
        assertEquals("INTAKE should be red for danger", android.R.color.holo_red_dark, intakeResource)
        
        // WORK is active but controlled - should be orange
        val workResource = BarrelStateColors.getColorResourceForState(BarrelState.WORK)
        assertEquals("WORK should be orange for active", android.R.color.holo_orange_dark, workResource)
        
        // EXHAUST is safe release - should be blue
        val exhaustResource = BarrelStateColors.getColorResourceForState(BarrelState.EXHAUST)
        assertEquals("EXHAUST should be blue for safe", android.R.color.holo_blue_dark, exhaustResource)
        
        // WAIT_FOR_INTAKE is safe waiting - should be green
        val waitIntakeResource = BarrelStateColors.getColorResourceForState(BarrelState.WAIT_FOR_INTAKE)
        assertEquals("WAIT_FOR_INTAKE should be green for safe", android.R.color.holo_green_dark, waitIntakeResource)
        
        // WAIT_FOR_WORK is pressurized and ready - should be purple
        val waitWorkResource = BarrelStateColors.getColorResourceForState(BarrelState.WAIT_FOR_WORK)
        assertEquals("WAIT_FOR_WORK should be purple for pressurized", android.R.color.holo_purple, waitWorkResource)
    }
}
