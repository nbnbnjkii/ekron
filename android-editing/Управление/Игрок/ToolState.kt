package com.example.realms

enum class HandTool { HAND_EMPTY, SPAWN_GUN, GRAVITY_GUN, DELETE_TOOL, DRONE_REMOTE }

/** Pure state reducer; held state is confirmed by the native world, never guessed on tap. */
class ToolState {
    var tool = HandTool.HAND_EMPTY
        private set
    var holding = false
        private set
    fun select(next: HandTool) { tool = next; holding = false }
    fun nextTool(): HandTool = when(tool) {
        HandTool.HAND_EMPTY -> HandTool.SPAWN_GUN
        HandTool.SPAWN_GUN -> HandTool.GRAVITY_GUN
        else -> HandTool.HAND_EMPTY
    }
    fun confirmHolding(value: Boolean) { holding = tool == HandTool.GRAVITY_GUN && value }
    val viewmodelVisible get() = tool == HandTool.SPAWN_GUN || tool == HandTool.GRAVITY_GUN
    val spawnVisible get() = tool == HandTool.SPAWN_GUN
    val gravityVisible get() = tool == HandTool.GRAVITY_GUN
    val rotateHeldVisible get() = gravityVisible && holding
    val deleteVisible get() = tool == HandTool.SPAWN_GUN
    val grabCaption get() = if (holding) "Бросить" else "Захват"
}
