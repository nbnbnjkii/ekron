package com.example.realms

import android.content.Context
import android.graphics.*
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.*
import kotlin.math.*

/** Edits normalized offsets and visual/touch scale, keeping native pointer handling intact. */
class ControlEditorView(context:Context, root:ViewGroup, private val profile:String, private val done:()->Unit):android.widget.FrameLayout(context){
    data class Target(val view:View,val key:String,val label:String,val rect:RectF,var dx:Float,var dy:Float,var scale:Float)
    private val prefs=context.getSharedPreferences("controls-$profile",0)
    private val targets=mutableListOf<Target>()
    private val paint=Paint(Paint.ANTI_ALIAS_FLAG)
    private var selected:Target?=null
    private var lastX=0f;private var lastY=0f
    private val sourceWidth=root.width.toFloat().coerceAtLeast(1f)
    private val sourceHeight=root.height.toFloat().coerceAtLeast(1f)
    private val density=resources.displayMetrics.density
    private fun dp(v:Float)=v*density
    private val header=dp(56f)
    private val footer=dp(128f)
    private var ratio=1f;private var originX=0f;private var originY=0f
    private val decrease=Button(context)
    private val increase=Button(context)
    private val selection=Spinner(context)
    private var dragId=-1
    init {
        val origin=IntArray(2);root.getLocationOnScreen(origin)
        fun scan(group:ViewGroup,path:String,inDrone:Boolean=false){for(i in 0 until group.childCount){val v=group.getChildAt(i);val key="$path/$i"
            val control=v is HudButton||v is Button||v is StickView
            if(control&&v.width>0&&v.height>0){
                val label=v.contentDescription?.toString()?.takeIf { it.isNotBlank() }?:if(v is Button)v.text.toString() else v.javaClass.simpleName
                val relevant=when(profile){"drone"->inDrone||label.contains("FPV")||label.contains("дрон",true);"car"->!inDrone&&(v is StickView||label.contains("Сесть")||label.contains("Выйти")||label.contains("вид",true));else->!inDrone}
                if(relevant){val pos=IntArray(2);v.getLocationOnScreen(pos)
                    val dx=prefs.getFloat("$key.x",0f);val dy=prefs.getFloat("$key.y",0f)
                    val cx=pos[0]-origin[0]-v.translationX+v.width*(v.scaleX-1)/2;val cy=pos[1]-origin[1]-v.translationY+v.height*(v.scaleY-1)/2
                    targets.add(Target(v,key,label,RectF(cx,cy,cx+v.width,cy+v.height),dx,dy,prefs.getFloat("$key.s",1f)))
                }
            }else if(v is ViewGroup)scan(v,key,inDrone||v is FpvControlView)
        }}
        scan(root,"")
        setWillNotDraw(false);isMotionEventSplittingEnabled=true
        val toolbar=LinearLayout(context).apply { orientation=LinearLayout.VERTICAL;setPadding(dp(8f).toInt(),dp(4f).toInt(),dp(8f).toInt(),dp(4f).toInt());setBackgroundColor(0xff182a22.toInt()) }
        fun row()=LinearLayout(context).also { toolbar.addView(it,LinearLayout.LayoutParams(-1,0,1f)) }
        fun button(row:LinearLayout,button:Button,text:String,click:()->Unit){
            button.text=text;button.textSize=16f;button.minHeight=dp(56f).toInt();button.setTextColor(Color.WHITE)
            button.setOnClickListener { click() };row.addView(button,LinearLayout.LayoutParams(0,-1,1f).apply { setMargins(dp(3f).toInt(),dp(2f).toInt(),dp(3f).toInt(),dp(2f).toInt()) })
        }
        val sizing=row()
        button(sizing,decrease,"− Уменьшить"){resize(-.1f)}
        button(sizing,increase,"+ Увеличить"){resize(.1f)}
        val actions=row()
        button(actions,Button(context),"Сброс раскладки"){for(t in targets){t.dx=0f;t.dy=0f;t.scale=1f};invalidate()}
        button(actions,Button(context),"Сохранить и выйти"){commit();done()}
        addView(toolbar,LayoutParams(-1,footer.toInt(),android.view.Gravity.BOTTOM))
        selection.adapter=ArrayAdapter(context,android.R.layout.simple_spinner_dropdown_item,listOf("Выберите кнопку для изменения")+targets.map { it.label })
        selection.setBackgroundColor(0xffdce8df.toInt())
        selection.onItemSelectedListener=object:AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent:AdapterView<*>?,view:View?,position:Int,id:Long){selected=targets.getOrNull(position-1);updateSelection()}
            override fun onNothingSelected(parent:AdapterView<*>?){selected=null;updateSelection()}
        }
        addView(selection,LayoutParams(-1,header.toInt(),android.view.Gravity.TOP));updateSelection()
    }
    private fun updateSelection(){decrease.isEnabled=selected!=null;increase.isEnabled=selected!=null;invalidate()}
    private fun resize(delta:Float){selected?.let { t->t.scale=(t.scale+delta).coerceIn(.6f,2f);keepInside(t) };updateSelection()}
    private fun keepInside(t:Target){
        val hw=(t.rect.width()*t.scale/2).coerceAtMost(sourceWidth/2)
        val hh=(t.rect.height()*t.scale/2).coerceAtMost(sourceHeight/2)
        t.dx=((t.rect.centerX()+t.dx*sourceWidth).coerceIn(hw,sourceWidth-hw)-t.rect.centerX())/sourceWidth
        t.dy=((t.rect.centerY()+t.dy*sourceHeight).coerceIn(hh,sourceHeight-hh)-t.rect.centerY())/sourceHeight
    }
    private fun bounds(t:Target):RectF{val cx=t.rect.centerX()+t.dx*sourceWidth;val cy=t.rect.centerY()+t.dy*sourceHeight;return RectF(cx-t.rect.width()*t.scale/2,cy-t.rect.height()*t.scale/2,cx+t.rect.width()*t.scale/2,cy+t.rect.height()*t.scale/2)}
    private fun commit(){val e=prefs.edit();for(t in targets){e.putFloat("${t.key}.x",t.dx).putFloat("${t.key}.y",t.dy).putFloat("${t.key}.s",t.scale)};e.apply()}
    override fun onDraw(c:Canvas){
        c.drawColor(0xff101b17.toInt())
        ratio=min(width/sourceWidth,((height-header-footer).coerceAtLeast(dp(48f)))/sourceHeight)
        originX=(width-sourceWidth*ratio)/2;originY=header+(height-header-footer-sourceHeight*ratio)/2
        c.save();c.translate(originX,originY);c.scale(ratio,ratio)
        paint.color=0xff26362d.toInt();c.drawRect(0f,0f,sourceWidth,sourceHeight,paint)
        for(t in targets){val r=bounds(t)
            c.save();c.translate(r.left,r.top);c.scale(t.scale,t.scale);t.view.draw(c);c.restore()
            if(t==selected){paint.color=0xff92ffd0.toInt();paint.style=Paint.Style.STROKE;paint.strokeWidth=dp(3f)/ratio;c.drawRoundRect(r,dp(8f),dp(8f),paint);paint.style=Paint.Style.FILL}
        }
        c.restore()
        selected?.let { paint.color=Color.WHITE;paint.textSize=dp(14f);c.drawText("Размер ${(it.scale*100).toInt()}% · перетащите выделенную кнопку",dp(12f),height-footer-dp(4f),paint) }
    }
    override fun onTouchEvent(e:MotionEvent):Boolean{
        val px=(e.x-originX)/ratio;val py=(e.y-originY)/ratio
        when(e.actionMasked){
            MotionEvent.ACTION_DOWN->{
                if(e.y<header||e.y>=height-footer)return false
                val hit=targets.asReversed().firstOrNull { val r=bounds(it);r.inset(-dp(12f)/ratio,-dp(12f)/ratio);r.contains(px,py) }
                selected=hit;selection.setSelection(if(hit==null)0 else targets.indexOf(hit)+1)
                dragId=e.getPointerId(0);lastX=px;lastY=py;updateSelection()
            }
            MotionEvent.ACTION_MOVE->{val i=e.findPointerIndex(dragId);if(i>=0){val x=(e.getX(i)-originX)/ratio;val y=(e.getY(i)-originY)/ratio
                selected?.let { t->t.dx+=(x-lastX)/sourceWidth;t.dy+=(y-lastY)/sourceHeight;keepInside(t) };lastX=x;lastY=y;invalidate()}}
            MotionEvent.ACTION_UP->{dragId=-1;performClick()}
            MotionEvent.ACTION_CANCEL->dragId=-1
        };return true
    }
    override fun performClick():Boolean{super.performClick();return true}
    companion object {
        fun apply(root:ViewGroup,profile:String){val prefs=root.context.getSharedPreferences("controls-$profile",0)
            fun scan(group:ViewGroup,path:String){for(i in 0 until group.childCount){val v=group.getChildAt(i);val key="$path/$i"
                if(v is HudButton||v is Button||v is StickView){
                    v.translationX=prefs.getFloat("$key.x",0f)*root.width;v.translationY=prefs.getFloat("$key.y",0f)*root.height
                    val s=prefs.getFloat("$key.s",1f);v.scaleX=s;v.scaleY=s
                }else if(v is ViewGroup)scan(v,key)
            }};scan(root,"")
        }
    }
}
