package com.example.realms

import android.animation.ValueAnimator
import android.content.Context
import android.graphics.*
import android.view.MotionEvent
import android.view.View
import android.view.animation.DecelerateInterpolator
import kotlin.math.*

/** Passive HUD layer: the switch button owns its pointer until release/cancel. */
class ToolWheelView(context: Context, private val choose: (HandTool) -> Unit) : View(context) {
    private val density = resources.displayMetrics.density
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val icons = ToolWheelSelection.tools.map { tool -> ToolArtView(context).apply { this.tool = tool } }
    private val names = listOf("Грави-пушка", "Пульт FPV", "Пустые руки")
    private var progress = 0f
    private var animator: ValueAnimator? = null
    private var pointer = -1
    private var originX = 0f
    private var originY = 0f
    private var selected: Int? = null
    private var current = HandTool.HAND_EMPTY

    init { visibility = GONE; importantForAccessibility = IMPORTANT_FOR_ACCESSIBILITY_NO }
    private fun fade(show: Boolean) {
        animator?.cancel()
        if (show) visibility = VISIBLE
        animator = ValueAnimator.ofFloat(progress, if (show) 1f else 0f).apply {
            duration = if (show) 150L else 120L
            interpolator = DecelerateInterpolator()
            addUpdateListener {
                progress = it.animatedValue as Float
                if (progress == 0f && pointer < 0) visibility = GONE
                invalidate()
            }
            start()
        }
    }
    fun cancel() { pointer = -1; selected = null; fade(false) }
    fun gesture(event: MotionEvent, tool: HandTool, open: () -> Unit): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                open(); current = tool; pointer = event.getPointerId(0)
                originX = event.x; originY = event.y; selected = null; fade(true)
            }
            MotionEvent.ACTION_MOVE -> {
                val i = event.findPointerIndex(pointer)
                if (i < 0) cancel() else {
                    val next = ToolWheelSelection.sector(event.getX(i) - originX, event.getY(i) - originY, 20f * density)
                    if (next != selected) { selected = next; invalidate() }
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                if (event.getPointerId(event.actionIndex) == pointer) {
                    val i = event.actionIndex
                    val index = ToolWheelSelection.sector(event.getX(i) - originX, event.getY(i) - originY, 20f * density)
                    cancel()
                    if (index != null) choose(ToolWheelSelection.tools[index])
                }
            }
            MotionEvent.ACTION_CANCEL -> cancel()
        }
        return true
    }
    override fun onDetachedFromWindow() { animator?.cancel(); pointer = -1; super.onDetachedFromWindow() }
    override fun onDraw(canvas: Canvas) {
        val radius = min(158f * density, min(width * .32f, height * .43f))
        if (radius <= 0) return
        val cx = width - radius - 76f * density
        val cy = height / 2f
        canvas.saveLayerAlpha(0f, 0f, width.toFloat(), height.toFloat(), (progress * 255).toInt())
        canvas.scale(.9f + .1f * progress, .9f + .1f * progress, cx, cy)
        val outer = RectF(cx-radius, cy-radius, cx+radius, cy+radius)
        val innerRadius = radius * .36f
        val inner = RectF(cx-innerRadius, cy-innerRadius, cx+innerRadius, cy+innerRadius)
        for (i in ToolWheelSelection.tools.indices) {
            val start = -150f + i * 120 + 2
            val path = Path().apply {
                arcTo(outer, start, 116f)
                val end = Math.toRadians((start + 116).toDouble())
                lineTo(cx + cos(end).toFloat()*innerRadius, cy + sin(end).toFloat()*innerRadius)
                arcTo(inner, start + 116, -116f); close()
            }
            val active = selected == i
            paint.style = Paint.Style.FILL
            paint.color = if (active) 0xbb956124.toInt() else 0xb320303d.toInt()
            canvas.drawPath(path, paint)
            paint.style = Paint.Style.STROKE; paint.strokeWidth = (if (active) 2.5f else 1f)*density
            paint.color = if (active) 0xffffb652.toInt() else 0xaa9ddbe9.toInt()
            canvas.drawPath(path, paint)
            val angle = Math.toRadians((-90 + i*120).toDouble())
            val x = cx + cos(angle).toFloat()*radius*.69f
            val y = cy + sin(angle).toFloat()*radius*.69f
            val iw = radius*.62f; val ih = iw*.625f
            canvas.save(); canvas.translate(x-iw/2, y-ih/2-8*density)
            icons[i].layout(0,0,iw.toInt(),ih.toInt()); icons[i].draw(canvas); canvas.restore()
            paint.style = Paint.Style.FILL; paint.color = Color.WHITE
            paint.textAlign = Paint.Align.CENTER; paint.textSize = min(12f*density, radius*.085f)
            canvas.drawText(names[i], x, y+ih*.5f+5*density, paint)
            if (ToolWheelSelection.tools[i] == current) canvas.drawCircle(x, y+ih*.5f+12*density, 2*density, paint)
        }
        paint.style = Paint.Style.FILL; paint.color = Color.WHITE; paint.textAlign = Paint.Align.CENTER
        paint.textSize = min(11f*density, radius*.08f)
        canvas.drawText(selected?.let { names[it] } ?: "Выбор", cx, cy-2*density, paint)
        paint.textSize = min(9f*density, radius*.065f)
        canvas.drawText(if (selected == null) "Проведите пальцем" else "Отпустите", cx, cy+14*density, paint)
        canvas.restore()
    }
}
