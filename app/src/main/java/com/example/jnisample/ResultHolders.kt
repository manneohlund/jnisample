package com.example.jnisample

import android.graphics.Color
import android.util.TypedValue
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import smartadapter.viewholder.SmartViewHolder

/**
 * Created by Manne Öhlund on 2019-04-10.
 */

class ResultHolder(parentView: ViewGroup) :
    SmartViewHolder<String>(LayoutInflater.from(parentView.context).inflate(R.layout.simple_text_view, parentView, false)) {

    override fun bind(item: String) {
        (itemView as TextView).text = item
    }
}

class ErrorResultHolder(parentView: ViewGroup) :
    SmartViewHolder<ErrorString>(inflateErrorTextView(parentView)) {

    override fun bind(item: ErrorString) {
        (itemView as TextView).text = item.string
    }

    companion object {
        fun inflateErrorTextView(parentView: ViewGroup): View {
            val textView = LayoutInflater.from(parentView.context).inflate(R.layout.simple_text_view, parentView, false)
            (textView as TextView).setTextColor(Color.RED)
            return textView
        }
    }
}

data class ErrorString(val string: String);

class CommandHolder(parentView: ViewGroup) :
    SmartViewHolder<CommandString>(inflateCommandTextView(parentView)) {

    override fun bind(item: CommandString) {
        (itemView as TextView).text = item.command
    }

    companion object {
        fun inflateCommandTextView(parentView: ViewGroup): View {
            val textView = LayoutInflater.from(parentView.context).inflate(R.layout.simple_text_view, parentView, false)
            (textView as TextView).setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
            textView.setPadding(20, 20, 20, 20);
            return textView
        }
    }
}

class SavedCommandHolder(parentView: ViewGroup) :
    SmartViewHolder<SavedCommandString>(inflateCommandTextView(parentView)) {

    override fun bind(item: SavedCommandString) {
        (itemView as TextView).text = item.command
    }

    companion object {
        fun inflateCommandTextView(parentView: ViewGroup): View {
            val textView = LayoutInflater.from(parentView.context).inflate(R.layout.simple_text_view, parentView, false)
            (textView as TextView).setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
            textView.setPadding(20, 20, 20, 20);
            return textView
        }
    }
}

data class CommandString(val command: String);

data class SavedCommandString(val command: String);