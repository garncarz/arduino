package cz.garncarz.compressed_air_controller.util

import android.content.Context
import androidx.core.content.ContextCompat
import cz.garncarz.compressed_air_controller.model.BarrelState

/**
 * Centralized color management for barrel states
 * Provides consistent colors across the entire application
 */
object BarrelStateColors {
    
    /**
     * Get the color for a specific barrel state
     */
    fun getColorForState(context: Context, state: BarrelState): Int {
        return when (state) {
            BarrelState.INTAKE -> ContextCompat.getColor(context, android.R.color.holo_red_dark) // Red - dangerous
            BarrelState.WORK -> ContextCompat.getColor(context, android.R.color.holo_orange_dark) // Orange
            BarrelState.EXHAUST -> ContextCompat.getColor(context, android.R.color.holo_blue_dark) // Blue
            BarrelState.WAIT_FOR_INTAKE -> ContextCompat.getColor(context, android.R.color.holo_green_dark) // Green
            BarrelState.WAIT_FOR_WORK -> ContextCompat.getColor(context, android.R.color.holo_purple) // Purple - pressurized
        }
    }
    
    /**
     * Color map for easy access in different contexts
     */
    private val STATE_COLOR_MAP = mapOf(
        BarrelState.INTAKE to android.R.color.holo_red_dark,
        BarrelState.WORK to android.R.color.holo_orange_dark,
        BarrelState.EXHAUST to android.R.color.holo_blue_dark,
        BarrelState.WAIT_FOR_INTAKE to android.R.color.holo_green_dark,
        BarrelState.WAIT_FOR_WORK to android.R.color.holo_purple
    )
    
    /**
     * Get the Android color resource ID for a state
     */
    fun getColorResourceForState(state: BarrelState): Int {
        return STATE_COLOR_MAP[state] ?: android.R.color.darker_gray
    }
}
