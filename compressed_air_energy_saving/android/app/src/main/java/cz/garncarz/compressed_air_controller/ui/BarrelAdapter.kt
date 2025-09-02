package cz.garncarz.compressed_air_controller.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import cz.garncarz.compressed_air_controller.R
import cz.garncarz.compressed_air_controller.model.BarrelData
import cz.garncarz.compressed_air_controller.model.BarrelState
import cz.garncarz.compressed_air_controller.util.BarrelStateColors

class BarrelAdapter(
    private val onStateClicked: ((BarrelData, View) -> Unit)? = null
) : RecyclerView.Adapter<BarrelAdapter.BarrelViewHolder>() {
    private var barrels: List<BarrelData> = emptyList()

    fun updateBarrels(newBarrels: List<BarrelData>) {
        barrels = newBarrels
        notifyDataSetChanged()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): BarrelViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_barrel, parent, false)
        return BarrelViewHolder(view)
    }

    override fun onBindViewHolder(holder: BarrelViewHolder, position: Int) {
        holder.bind(barrels[position])
    }

    override fun getItemCount(): Int = barrels.size

    inner class BarrelViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val barrelNameText: TextView = itemView.findViewById(R.id.barrelNameText)
        private val barrelStateText: TextView = itemView.findViewById(R.id.barrelStateText)
        private val pressureText: TextView = itemView.findViewById(R.id.pressureText)
        private val upperSensorText: TextView = itemView.findViewById(R.id.upperSensorText)
        private val lowerSensorText: TextView = itemView.findViewById(R.id.lowerSensorText)

        fun bind(barrel: BarrelData) {
            barrelNameText.text = "Barrel ${barrel.id}"
            barrelStateText.text = barrel.state.name
            pressureText.text = if (barrel.pressureReading != null) "${barrel.pressureReading.toInt()}" else "--"
            upperSensorText.text = formatSensorValue(barrel.upperWaterSensor)
            lowerSensorText.text = formatSensorValue(barrel.lowerWaterSensor)

            // Set state background color based on state
            val context = itemView.context
            val backgroundColor = BarrelStateColors.getColorForState(context, barrel.state)
            barrelStateText.background?.let { drawable ->
                drawable.setTint(backgroundColor)
            }

            // Allow users to tap the state pill to change state (in MANUAL mode)
            barrelStateText.isClickable = true
            barrelStateText.isLongClickable = false
            barrelStateText.isFocusable = false
            barrelStateText.isPressed = false
            barrelStateText.setOnClickListener {
                onStateClicked?.invoke(barrel, barrelStateText)
            }
        }

        private fun formatSensorValue(value: Boolean?): String {
            return when (value) {
                true -> "WET"
                false -> "DRY"
                null -> "--"
            }
        }
    }
}
