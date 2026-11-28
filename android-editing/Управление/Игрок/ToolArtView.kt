package com.example.realms

import android.content.Context
import android.graphics.*
import android.os.SystemClock
import android.view.View
import kotlin.math.sin

/** Original vector-painted 2D viewmodels, shared with backpack icons; never consumes look gestures. */
class ToolArtView(context: Context, private val firstPerson: Boolean = false) : View(context) {
    var tool=HandTool.HAND_EMPTY
        set(value) { if(field!=value){field=value;invalidate()} }
    var sprinting=false
    var moving=false
    var running=false
        set(value) { field=value;if(value)invalidate() }
    private val p=Paint(Paint.ANTI_ALIAS_FLAG)
    override fun onDraw(c: Canvas) {
        if(firstPerson && (tool==HandTool.HAND_EMPTY || tool==HandTool.DELETE_TOOL))return
        c.save();c.scale(width/240f,height/150f)
        if(firstPerson) {
            val phase=SystemClock.uptimeMillis()/1000f*8
            if(running && moving)c.translate(sin(phase)*(if(sprinting)2.5f else .8f),sin(phase*2)*(if(sprinting)1.8f else .5f))
        }
        val glow=when(tool){HandTool.SPAWN_GUN->Color.rgb(71,231,210);HandTool.GRAVITY_GUN->Color.rgb(255,173,54);else->Color.rgb(255,92,99)}
        fun polygon(vararg points: Float) {val path=Path();path.moveTo(points[0],points[1]);for(i in 2 until points.size step 2)path.lineTo(points[i],points[i+1]);path.close();c.drawPath(path,p)}
        if(tool==HandTool.DRONE_REMOTE) {
            p.color=0xff26363d.toInt();c.drawRoundRect(32f,40f,208f,130f,18f,18f,p)
            p.color=0xff7399a5.toInt();c.drawRoundRect(91f,52f,149f,90f,4f,4f,p)
            p.color=Color.LTGRAY;c.drawCircle(64f,92f,19f,p);c.drawCircle(176f,92f,19f,p)
            p.color=Color.BLACK;c.drawCircle(64f,92f,8f,p);c.drawCircle(176f,92f,8f,p)
            p.color=Color.LTGRAY;p.strokeWidth=7f;c.drawLine(48f,42f,38f,9f,p);c.drawLine(192f,42f,202f,9f,p)
            c.restore();return
        }
        if(tool==HandTool.HAND_EMPTY) {
            p.color=Color.LTGRAY;p.style=Paint.Style.STROKE;p.strokeWidth=8f;p.strokeCap=Paint.Cap.ROUND
            c.drawRoundRect(92f,50f,151f,127f,18f,18f,p)
            for(i in 0..3)c.drawLine(95f+i*17,77f,95f+i*17,23f+(i%2)*7,p)
            p.style=Paint.Style.FILL;c.restore();return
        }
        if(tool==HandTool.DELETE_TOOL){
            p.color=0xff482b35.toInt();c.drawRoundRect(76f,37f,166f,133f,12f,12f,p)
            p.color=glow;c.drawRoundRect(64f,25f,178f,39f,5f,5f,p);c.drawRoundRect(104f,11f,140f,25f,4f,4f,p)
            p.color=Color.WHITE;p.strokeWidth=7f;for(i in 0..2)c.drawLine(99f+i*23,57f,99f+i*23,114f,p)
            c.restore();return
        }
        // Gloved hand, grip, layered chassis and polished barrel.
        p.shader=LinearGradient(120f,70f,215f,148f,intArrayOf(0xff535d64.toInt(),0xff141c24.toInt()),null,Shader.TileMode.CLAMP)
        polygon(156f,77f,196f,86f,224f,150f,165f,150f,137f,101f)
        p.shader=null;p.color=0xff0d151d.toInt();polygon(148f,66f,172f,72f,161f,122f,140f,115f)
        p.shader=LinearGradient(0f,35f,0f,95f,intArrayOf(0xff9aaeb6.toInt(),0xff324653.toInt(),0xff111c27.toInt()),null,Shader.TileMode.CLAMP)
        polygon(48f,43f,131f,33f,194f,63f,180f,99f,106f,93f,47f,69f)
        p.shader=null;p.color=0xffd7e0e4.toInt();polygon(68f,39f,132f,30f,182f,54f,126f,53f)
        p.color=0xff172b37.toInt();c.drawRoundRect(91f,56f,148f,79f,5f,5f,p)
        p.color=glow;c.drawRoundRect(99f,61f,139f,67f,3f,3f,p)
        for(i in 0..3){p.color=0xff142430.toInt();c.drawRect(153f+i*7,65f,157f+i*7,80f,p)}
        if(tool==HandTool.GRAVITY_GUN){
            p.style=Paint.Style.STROKE;p.strokeWidth=10f;p.strokeCap=Paint.Cap.ROUND;p.color=0xff71838a.toInt()
            c.drawLine(80f,43f,38f,20f,p);c.drawLine(38f,20f,18f,34f,p);c.drawLine(73f,77f,31f,90f,p);c.drawLine(31f,90f,14f,72f,p)
            p.strokeWidth=4f;p.color=glow;c.drawLine(38f,20f,18f,34f,p);c.drawLine(31f,90f,14f,72f,p)
            p.style=Paint.Style.FILL;p.color=0x55ffab3d;c.drawCircle(29f,53f,24f,p);p.color=glow;c.drawCircle(29f,53f,8f,p)
        }else{
            p.color=0xff243d4b.toInt();c.drawRoundRect(28f,40f,75f,71f,8f,8f,p)
            p.color=glow;c.drawRoundRect(26f,45f,33f,66f,3f,3f,p)
            p.color=0x5547e7d2;c.drawCircle(40f,18f,22f,p)
            p.style=Paint.Style.STROKE;p.strokeWidth=2.5f;p.color=glow
            polygon(40f,0f,56f,9f,56f,27f,40f,36f,24f,27f,24f,9f)
            c.drawLine(24f,9f,40f,18f,p);c.drawLine(56f,9f,40f,18f,p);c.drawLine(40f,18f,40f,36f,p);p.style=Paint.Style.FILL
        }
        c.restore()
        if(firstPerson && running && moving)postInvalidateOnAnimation()
    }
}
