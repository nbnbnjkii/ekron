package com.example.realms

/** Independent pointer ownership for one control. Other fingers cannot end or move this drag. */
class PointerDrag {
    var pointer = -1
        private set
    private var x = 0f
    private var y = 0f
    fun start(id: Int, nextX: Float, nextY: Float) { pointer=id;x=nextX;y=nextY }
    fun move(id: Int, nextX: Float, nextY: Float): Pair<Float, Float>? {
        if(id!=pointer || pointer<0)return null
        val delta=Pair(nextX-x,nextY-y);x=nextX;y=nextY;return delta
    }
    fun end(id: Int) { if(id==pointer)reset() }
    fun reset() { pointer=-1 }
}
