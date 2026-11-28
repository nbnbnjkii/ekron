package com.example.realms

import android.content.Context
import android.graphics.*
import android.view.Gravity
import android.view.View
import android.widget.FrameLayout
import kotlin.math.max

/** Mode 2 radio: left throttle/yaw, right pitch/roll. Independent pointer owners. */
class FpvControlView(context: Context, private val input: (Float,Float,Float,Float)->Unit, private val look:(Float,Float)->Unit) : FrameLayout(context) {
    private var throttle=0f
    private var yaw=0f
    private var pitch=0f
    private var roll=0f
    private val leftStick=StickView(context){x,y->yaw=x;throttle=y;send()}
    private val rightStick=StickView(context){x,y->roll=x;pitch=y;send()}
    private val cameraLook=LookView(context,0f){x,y->look(x,y)}
    private fun send(){input(throttle,yaw,pitch,roll)}
    init {
        isMotionEventSplittingEnabled=true;isClickable=true
        val density=resources.displayMetrics.density
        addView(object:View(context){
            private val paint=Paint(Paint.ANTI_ALIAS_FLAG)
            override fun onSizeChanged(w:Int,h:Int,oldw:Int,oldh:Int){paint.shader=RadialGradient(w/2f,h/2f,max(w,h)*.65f,intArrayOf(Color.TRANSPARENT,0x99050b10.toInt()),floatArrayOf(.35f,1f),Shader.TileMode.CLAMP)}
            override fun onDraw(canvas:Canvas){canvas.drawRect(0f,0f,width.toFloat(),height.toFloat(),paint)}
        },LayoutParams(-1,-1))
        addView(cameraLook,LayoutParams(-1,-1))
        val size=(170*density).toInt()
        leftStick.contentDescription="FPV левый джойстик: вверх и вниз — высота, влево и вправо — рыскание"
        rightStick.contentDescription="FPV правый джойстик: вверх и вниз — продольный наклон, влево и вправо — боковой наклон"
        addView(leftStick,LayoutParams(size,size,Gravity.BOTTOM or Gravity.START).apply {leftMargin=(18*density).toInt();bottomMargin=(18*density).toInt()})
        addView(rightStick,LayoutParams(size,size,Gravity.BOTTOM or Gravity.END).apply {rightMargin=(18*density).toInt();bottomMargin=(18*density).toInt()})
    }
    fun reset(){cameraLook.reset();leftStick.reset();rightStick.reset();throttle=0f;yaw=0f;pitch=0f;roll=0f;send()}
    override fun onDetachedFromWindow(){reset();super.onDetachedFromWindow()}
}
