package com.example.realms

import android.content.Context
import android.graphics.*
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import kotlin.math.*

/** Edits normalized offsets and visual/touch scale, keeping native pointer handling intact. */
class ControlEditorView(context:Context, root:ViewGroup, private val profile:String, private val done:()->Unit):View(context){
    data class Target(val view:View,val key:String,val label:String,val rect:RectF,var dx:Float,var dy:Float,var scale:Float)
    private val prefs=context.getSharedPreferences("controls-$profile",0)
    private val targets=mutableListOf<Target>()
    private val paint=Paint(Paint.ANTI_ALIAS_FLAG)
    private var selected:Target?=null
    private var lastX=0f;private var lastY=0f
    init {
        val origin=IntArray(2);root.getLocationOnScreen(origin)
        fun scan(group:ViewGroup,path:String,inDrone:Boolean=false){for(i in 0 until group.childCount){val v=group.getChildAt(i);val key="$path/$i"
            val control=v is HudButton||v is Button||v is StickView
            if(control&&v.width>0&&v.height>0){
                val label=v.contentDescription?.toString()?.takeIf { it.isNotBlank() }?:if(v is Button)v.text.toString() else v.javaClass.simpleName
                val relevant=when(profile){"drone"->inDrone||label.contains("FPV")||label.contains("дрон",true);"car"->!inDrone&&(v is StickView||label.contains("Сесть")||label.contains("Выйти")||label.contains("вид",true));else->!inDrone}
                if(relevant){val pos=IntArray(2);v.getLocationOnScreen(pos)
                    val dx=prefs.getFloat("$key.x",0f);val dy=prefs.getFloat("$key.y",0f)
                    val cx=pos[0]-origin[0]-v.translationX;val cy=pos[1]-origin[1]-v.translationY
                    targets.add(Target(v,key,label,RectF(cx,cy,cx+v.width,cy+v.height),dx,dy,prefs.getFloat("$key.s",1f)))
                }
            }else if(v is ViewGroup)scan(v,key,inDrone||v is FpvControlView)
        }}
        scan(root,"")
    }
    private fun bounds(t:Target):RectF{val r=RectF(t.rect);r.offset(t.dx*width,t.dy*height);val cx=r.centerX();val cy=r.centerY();return RectF(cx-r.width()*t.scale/2,cy-r.height()*t.scale/2,cx+r.width()*t.scale/2,cy+r.height()*t.scale/2)}
    private fun commit(){val e=prefs.edit();for(t in targets){e.putFloat("${t.key}.x",t.dx).putFloat("${t.key}.y",t.dy).putFloat("${t.key}.s",t.scale)};e.apply()}
    override fun onDraw(c:Canvas){c.drawColor(0xf0182421.toInt());paint.color=Color.WHITE;paint.textSize=26f
        c.drawText("Управление: $profile · перетащите кнопку",20f,32f,paint)
        for(t in targets){val r=bounds(t);paint.color=if(t==selected)0xff477d65.toInt() else 0xff33463e.toInt();c.drawRoundRect(r,12f,12f,paint);paint.color=Color.WHITE;paint.textSize=18f;c.drawText(t.label.take(20),r.left+4,r.centerY(),paint)}
        paint.color=Color.WHITE;paint.textSize=25f;c.drawText("− Размер      + Размер      Сброс      Готово",20f,height-20f,paint)
    }
    override fun onTouchEvent(e:MotionEvent):Boolean{
        when(e.actionMasked){
            MotionEvent.ACTION_DOWN->{
                if(e.y>height-64){when((e.x/width*4).toInt()){
                    0->selected?.let { it.scale=(it.scale-.1f).coerceAtLeast(.6f) }
                    1->selected?.let { it.scale=(it.scale+.1f).coerceAtMost(2f) }
                    2->for(t in targets){t.dx=0f;t.dy=0f;t.scale=1f}
                    3->{commit();done();return true}
                };invalidate();return true}
                selected=targets.asReversed().firstOrNull { bounds(it).contains(e.x,e.y) };lastX=e.x;lastY=e.y;invalidate()
            }
            MotionEvent.ACTION_MOVE->{selected?.let { t->t.dx=((t.dx*width+e.x-lastX)/width).coerceIn(-.8f,.8f);t.dy=((t.dy*height+e.y-lastY)/height).coerceIn(-.8f,.8f) };lastX=e.x;lastY=e.y;invalidate()}
            MotionEvent.ACTION_UP->{commit();performClick()}
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
