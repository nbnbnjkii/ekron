package com.example.realms

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.view.MotionEvent
import android.view.View
import kotlin.math.*

/** Native Android controls: no bitmap assets, fixed pointer ownership, no frame polling for movement. */
class StickView(context: Context, private val move: (Float, Float) -> Unit) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private var pointer = -1
    private var dx = 0f
    private var dy = 0f
    init { contentDescription = "Джойстик движения"; isClickable = true }
    fun reset() { pointer = -1; dx = 0f; dy = 0f; move(0f, 0f); invalidate() }
    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> { pointer = event.getPointerId(0); parent.requestDisallowInterceptTouchEvent(true); update(event, 0) }
            MotionEvent.ACTION_MOVE -> { val index = event.findPointerIndex(pointer); if (index >= 0) update(event, index) else reset() }
            MotionEvent.ACTION_POINTER_UP -> if (event.getPointerId(event.actionIndex) == pointer) reset()
            MotionEvent.ACTION_UP -> { reset(); performClick() }
            MotionEvent.ACTION_CANCEL -> reset()
        }
        return true
    }
    override fun performClick(): Boolean { super.performClick(); return true }
    private fun update(event: MotionEvent, i: Int) {
        val radius = min(width, height) * .32f
        if (radius <= 0) return
        dx = (event.getX(i) - width / 2f) / radius
        dy = (event.getY(i) - height / 2f) / radius
        val length = hypot(dx, dy)
        if (length > 1) { dx /= length; dy /= length }
        val magnitude = min(1f, length)
        val scale = if (magnitude < .12f) 0f else (magnitude - .12f) / .88f / magnitude
        move(dx * scale, -dy * scale); invalidate()
    }
    override fun onDraw(canvas: Canvas) {
        val x = width / 2f; val y = height / 2f; val r = min(width, height) * .48f
        paint.style = Paint.Style.FILL; paint.color = 0x9940403a.toInt(); canvas.drawCircle(x, y, r, paint)
        paint.style = Paint.Style.STROKE; paint.strokeWidth = 2 * resources.displayMetrics.density; paint.color = 0xbbeeeeee.toInt(); canvas.drawCircle(x, y, r, paint)
        paint.style = Paint.Style.FILL
        for (angle in listOf(0.0, PI / 2, PI, PI * 1.5)) {
            val cx = x + cos(angle).toFloat() * r * .79f; val cy = y + sin(angle).toFloat() * r * .79f
            val path = Path(); val ux = cos(angle).toFloat(); val uy = sin(angle).toFloat(); val size = r * .105f
            path.moveTo(cx + ux * size, cy + uy * size)
            path.lineTo(cx - ux * size - uy * size, cy - uy * size + ux * size)
            path.lineTo(cx - ux * size + uy * size, cy - uy * size - ux * size); path.close(); canvas.drawPath(path, paint)
        }
        paint.color = 0x99ffffff.toInt(); canvas.drawCircle(x + dx * r * .48f, y + dy * r * .48f, r * .43f, paint)
        paint.style = Paint.Style.STROKE; paint.color = Color.WHITE; canvas.drawCircle(x + dx * r * .48f, y + dy * r * .48f, r * .43f, paint)
    }
}

class HudButton(context: Context, var caption: String, var symbol: String) : View(context) {
    var visualScale = 1f
    var danger = false
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    init { isClickable = true; isFocusable = true; contentDescription = caption }
    override fun setSelected(selected: Boolean) { super.setSelected(selected); invalidate() }
    override fun setPressed(pressed: Boolean) { super.setPressed(pressed); invalidate() }
    override fun onDraw(canvas: Canvas) {
        val size = min(width, height).toFloat()*visualScale; val cx = width / 2f; val cy = height / 2f
        paint.style = Paint.Style.FILL
        paint.color = if (isSelected || isPressed) 0x99526758.toInt() else if(danger) 0x999f2929.toInt() else 0x6630332f.toInt()
        val bounds = RectF(cx-size*.48f, cy-size*.48f, cx+size*.48f, cy+size*.48f)
        if (symbol == "pause") canvas.drawRoundRect(bounds, size*.1f, size*.1f, paint) else canvas.drawOval(bounds, paint)
        paint.style = Paint.Style.STROKE; paint.strokeWidth = size*.015f; paint.color = 0xbbffffff.toInt()
        if (symbol != "pause") canvas.drawOval(bounds, paint)
        canvas.save(); canvas.translate(cx-size*.5f, cy-size*.5f); canvas.scale(size/100, size/100)
        paint.color = Color.WHITE; paint.strokeWidth = 6f; paint.strokeCap = Paint.Cap.ROUND
        fun line(x: Float,y: Float,a: Float,b: Float) = canvas.drawLine(x,y,a,b,paint)
        when (symbol) {
            "backpack" -> { canvas.drawRoundRect(RectF(30f,27f,70f,65f),6f,6f,paint);canvas.drawRoundRect(RectF(40f,40f,60f,57f),3f,3f,paint);line(41f,25f,41f,18f);line(41f,18f,59f,18f);line(59f,18f,59f,25f) }
            "cube" -> { canvas.drawRect(RectF(31f,28f,62f,60f),paint);line(31f,28f,42f,18f);line(42f,18f,73f,18f);line(73f,18f,73f,50f);line(73f,50f,62f,60f);line(62f,28f,73f,18f) }
            "left", "right", "release" -> { val start=if(symbol=="left")70f else 30f;val end=100-start;line(start,40f,end,40f);line(end,40f,50f,22f);line(end,40f,50f,58f) }
            "rotate" -> { canvas.drawArc(RectF(28f,18f,72f,62f),40f,280f,false,paint);line(70f,24f,70f,40f);line(70f,40f,55f,35f) }
            "grab" -> { canvas.drawArc(RectF(30f,18f,70f,62f),0f,180f,false,paint);line(30f,40f,30f,20f);line(70f,40f,70f,20f) }

            "pause" -> { paint.style=Paint.Style.FILL; canvas.drawRect(29f,25f,42f,75f,paint); canvas.drawRect(58f,25f,71f,75f,paint) }
            "build" -> { line(33f,57f,65f,25f); line(48f,23f,69f,44f); line(58f,17f,76f,35f) }
            "freeze" -> { for(i in 0..5){val a=i*Math.PI/3;val dx=kotlin.math.cos(a).toFloat();val dy=kotlin.math.sin(a).toFloat();line(50f,42f,50+dx*25,42+dy*25);val x=50+dx*16;val y=42+dy*16;line(x,y,x-dx*6-dy*6,y-dy*6+dx*6);line(x,y,x-dx*6+dy*6,y-dy*6-dx*6)} }
            "home" -> { line(24f,45f,50f,22f);line(50f,22f,76f,45f);line(31f,41f,31f,71f);line(31f,71f,69f,71f);line(69f,71f,69f,41f);line(45f,71f,45f,53f);line(45f,53f,56f,53f);line(56f,53f,56f,71f) }
            "switch" -> { line(25f,34f,75f,34f);line(75f,34f,62f,21f);line(75f,34f,62f,47f);line(75f,65f,25f,65f);line(25f,65f,38f,52f);line(25f,65f,38f,78f) }
            "eye" -> { canvas.drawArc(RectF(24f,34f,76f,66f),0f,360f,false,paint);line(31f,42f,25f,37f);line(69f,42f,75f,37f);paint.style=Paint.Style.FILL;canvas.drawCircle(50f,50f,8f,paint);paint.style=Paint.Style.STROKE }
            "remove" -> { line(32f,32f,68f,68f);line(68f,32f,32f,68f) }
            "jump" -> { paint.style=Paint.Style.FILL; val path=Path();path.moveTo(50f,16f);path.lineTo(28f,42f);path.lineTo(42f,42f);path.lineTo(42f,68f);path.lineTo(58f,68f);path.lineTo(58f,42f);path.lineTo(72f,42f);path.close();canvas.drawPath(path,paint) }
            "crouch", "prone" -> {
                paint.style=Paint.Style.FILL
                if(symbol=="crouch") { canvas.drawCircle(55f,22f,7f,paint); paint.style=Paint.Style.STROKE; line(52f,34f,44f,48f);line(44f,48f,62f,52f);line(62f,52f,53f,65f);line(53f,65f,70f,65f);line(48f,37f,35f,52f);line(35f,52f,30f,64f) }
                else { canvas.drawCircle(25f,40f,7f,paint); paint.style=Paint.Style.STROKE;line(37f,48f,60f,53f);line(60f,53f,81f,53f);line(39f,48f,24f,57f);line(24f,57f,16f,53f);line(58f,53f,77f,62f) }
            }
        }
        canvas.restore()
        if (caption.isNotEmpty()) { paint.style=Paint.Style.FILL; paint.textAlign=Paint.Align.CENTER;paint.textSize=size*(if(caption.length>10).103f else .142f);paint.color=Color.WHITE;canvas.drawText(caption,cx,cy+size*.34f,paint) }
    }
}

/** Shares the SAME Android touch dispatcher as the stick. No hand-off to a second surface. */
class LookView(context: Context, private val startFraction: Float = .42f, private val look: (Float, Float) -> Unit) : View(context) {
    private val drag = PointerDrag()
    init { contentDescription = "Обзор: проведите пальцем справа" }
    fun reset() { drag.reset() }
    override fun onTouchEvent(event: MotionEvent): Boolean {
        when(event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                if(event.x < width * startFraction)return false
                drag.start(event.getPointerId(0),event.x,event.y)
                parent.requestDisallowInterceptTouchEvent(true)
            }
            MotionEvent.ACTION_MOVE -> {
                val i=event.findPointerIndex(drag.pointer)
                if(i>=0)drag.move(event.getPointerId(i),event.getX(i),event.getY(i))?.let { look(it.first,it.second) }
            }
            MotionEvent.ACTION_POINTER_UP -> drag.end(event.getPointerId(event.actionIndex))
            MotionEvent.ACTION_UP,MotionEvent.ACTION_CANCEL -> reset()
        }
        return true
    }
}
