package cz.garncarz.compressed_air_controller.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import cz.garncarz.compressed_air_controller.R
import java.text.SimpleDateFormat
import java.util.*

data class LogEntry(val timestamp: Long, val message: String)

class LogAdapter : RecyclerView.Adapter<LogAdapter.LogViewHolder>() {
    private val logs: MutableList<LogEntry> = mutableListOf()
    private val dateFormatter = SimpleDateFormat("HH:mm:ss", Locale.getDefault())
    private val maxLogs = 100

    fun addLog(message: String) {
        logs.add(0, LogEntry(System.currentTimeMillis(), message))
        if (logs.size > maxLogs) {
            logs.removeAt(logs.size - 1)
        }
        notifyItemInserted(0)
    }

    fun clearLogs() {
        logs.clear()
        notifyDataSetChanged()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): LogViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_log, parent, false)
        return LogViewHolder(view)
    }

    override fun onBindViewHolder(holder: LogViewHolder, position: Int) {
        holder.bind(logs[position])
    }

    override fun getItemCount(): Int = logs.size

    inner class LogViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val logTimeText: TextView = itemView.findViewById(R.id.logTimeText)
        private val logMessageText: TextView = itemView.findViewById(R.id.logMessageText)

        fun bind(logEntry: LogEntry) {
            logTimeText.text = dateFormatter.format(Date(logEntry.timestamp))
            logMessageText.text = logEntry.message
        }
    }
}
