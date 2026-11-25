package com.example.realms

import org.junit.Assert.*
import org.junit.Test

class ToolWheelSelectionTest {
    @Test fun gravityRemoteAndHandsAreEquippable() {
        assertEquals(listOf(HandTool.GRAVITY_GUN, HandTool.DRONE_REMOTE, HandTool.HAND_EMPTY), ToolWheelSelection.tools)
        assertFalse(ToolWheelSelection.tools.contains(HandTool.SPAWN_GUN))
        assertFalse(ToolWheelSelection.tools.contains(HandTool.DELETE_TOOL))
    }
    @Test fun swipeDirectionsMatchThirds() {
        assertEquals(0, ToolWheelSelection.sector(0f, -100f, 20f))
        assertEquals(1, ToolWheelSelection.sector(87f, 50f, 20f))
        assertEquals(2, ToolWheelSelection.sector(-87f, 50f, 20f))
    }
    @Test fun tapAndReturnToCenterCancelSelection() {
        assertNull(ToolWheelSelection.sector(0f, 0f, 20f))
        assertNull(ToolWheelSelection.sector(20f, 0f, 20f))
        assertNull(ToolWheelSelection.sector(Float.NaN, 0f, 20f))
    }
    @Test fun catalogCategoriesPartitionExistingItems() {
        val building=BuildCatalog.indicesForCategory(0)
        val furniture=BuildCatalog.indicesForCategory(1)
        assertEquals(BuildCatalog.entries.indices.toSet(), (building+furniture).toSet())
        assertTrue(building.intersect(furniture.toSet()).isEmpty())
        assertEquals(setOf(4,5,6,7,13,18,22,25,28), furniture.map { BuildCatalog.entries[it].nativeType }.toSet())
        assertTrue(BuildCatalog.indicesForCategory(2).isEmpty())
        assertTrue(BuildCatalog.indicesForCategory(3).isEmpty())
    }
}
