package com.example.realms

import kotlin.math.atan2
import kotlin.math.hypot

/** Screen-space angles: gravity above, hands below. */
object ToolWheelSelection {
    val tools = listOf(HandTool.GRAVITY_GUN, HandTool.DRONE_REMOTE, HandTool.HAND_EMPTY)
    fun sector(dx: Float, dy: Float, deadZone: Float): Int? {
        if (!dx.isFinite() || !dy.isFinite() || hypot(dx, dy) <= deadZone) return null
        val angle = (Math.toDegrees(atan2(dy.toDouble(), dx.toDouble())) + 150 + 360) % 360
        return (angle / 120).toInt()
    }
}
