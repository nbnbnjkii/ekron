package com.example.realms

import android.content.Context
import android.graphics.*
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.view.Gravity
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import kotlin.math.*

/** Normal lobby UI: 3D room as background, real buttons on top. */
class LobbyView(context: Context, private val resumeGame: () -> Unit, private val newGame: () -> Unit, private val saveGame: () -> Unit) : FrameLayout(context) {
    var canContinue=false
    private var page=0
    var gameReady=false
    private var frameLimit=context.getSharedPreferences("graphics",0).getInt("fps",60)
    private var muted=context.getSharedPreferences("settings",0).getBoolean("muted",false)
    private var intro:LobbyIntro?=null
    private var introPaused=false
    private var introError=false
    private var notice=""
    private var posX=0f;private var posZ=-3f;private var yaw=0f;private var pitch=0f
    private var eye=1.7f;private var jump=0f;private var vertical=0f;private var crouch=false
    private var right=0f;private var forward=0f
    private val lookControl=LookView(context){dx,dy->yaw-=dx*.004f;pitch=(pitch-dy*.004f).coerceIn(-.9f,.9f)}
    private val walkControl=StickView(context){r,f->right=r;forward=f}
    private val jumpControl=HudButton(context,"Прыжок","jump")
    private val crouchControl=HudButton(context,"Присесть","crouch")
    private lateinit var menu: LinearLayout
    private lateinit var titleText: TextView
    private lateinit var noticeText: TextView
    private val density=context.resources.displayMetrics.density
    private fun dp(n:Float)=(n*density).toInt()
    private fun menuButton(text:String,onClick:()->Unit):Button=Button(context).apply {
        this.text=text;isAllCaps=false;textSize=18f;minHeight=dp(64f)
        setOnClickListener { onClick() }
        background=android.graphics.drawable.GradientDrawable().apply { setColor(0xff2f5a45.toInt());cornerRadius=dp(10f).toFloat() }
        setTextColor(Color.WHITE)
        layoutParams=LinearLayout.LayoutParams(-1,dp(64f)).apply { bottomMargin=dp(12f) }
    }
    init {
        setWillNotDraw(false);isMotionEventSplittingEnabled=true
        intro=LobbyIntro(context){introError=true;dirty=true;invalidate()}
        addView(intro,LayoutParams(-1,-1))
        lookControl.onTap={sx,sy->}
        addView(lookControl,LayoutParams(-1,-1))
        menu=LinearLayout(context).apply { orientation=LinearLayout.VERTICAL;setPadding(dp(24f),dp(24f),dp(24f),dp(24f)) }
        val card=LinearLayout(context).apply {
            orientation=LinearLayout.VERTICAL
            setPadding(dp(24f),dp(20f),dp(24f),dp(20f))
            background=android.graphics.drawable.GradientDrawable().apply { setColor(0xe0182a22.toInt());cornerRadius=dp(16f).toFloat() }
        }
        titleText=TextView(context).apply { text="ЭКРОН";textSize=30f;setTextColor(0xff92dbad.toInt());gravity=Gravity.CENTER;typeface=Typeface.MONOSPACE }
        noticeText=TextView(context).apply { text="";textSize=14f;setTextColor(0xffd9e8dd.toInt());gravity=Gravity.CENTER;setPadding(0,dp(8f),0,dp(8f)) }
        card.addView(titleText,LinearLayout.LayoutParams(-1,-2))
        card.addView(noticeText,LinearLayout.LayoutParams(-1,-2))
        menu.addView(card,LinearLayout.LayoutParams(-1,-2))
        val scroll=ScrollView(context).apply { isFillViewport=true }
        scroll.addView(menu,FrameLayout.LayoutParams(-1,-2))
        addView(scroll,LayoutParams(-1,-1).apply { leftMargin=dp(24f);rightMargin=dp(24f);topMargin=dp(28f);bottomMargin=dp(28f) })
        buildMenu()
        fun dpi(n:Int)=(n*density).toInt()
        val stick=(resources.configuration.screenHeightDp*.32f).toInt().coerceIn(120,184)
        val size=(resources.configuration.screenHeightDp*.145f).toInt().coerceIn(60,64)
        addView(walkControl,LayoutParams(dpi(stick),dpi(stick),Gravity.BOTTOM or Gravity.START).apply { leftMargin=dpi(14);bottomMargin=dpi(22) })
        jumpControl.setOnClickListener { if(jump==0f&&!crouch)vertical=6.5f }
        jumpControl.setOnTouchListener { v,e->when(e.actionMasked){MotionEvent.ACTION_DOWN->{v.isPressed=true;v.performClick()};MotionEvent.ACTION_UP,MotionEvent.ACTION_CANCEL->v.isPressed=false};true }
        crouchControl.setOnClickListener { crouch=!crouch;crouchControl.isSelected=crouch }
        addView(jumpControl,LayoutParams(dpi(size),dpi(size),Gravity.BOTTOM or Gravity.END).apply { rightMargin=dpi(8);bottomMargin=dpi(8+size) })
        addView(crouchControl,LayoutParams(dpi(size),dpi(size),Gravity.BOTTOM or Gravity.END).apply { rightMargin=dpi(8);bottomMargin=dpi(8) })
    }
    private fun clearMenu(){ while(menu.childCount>1)menu.removeViewAt(menu.childCount-1) }
    private fun buildMenu(){
        clearMenu()
        noticeText.text=notice
        when(page){
            0->{
                titleText.text=if(canContinue)"ЭКРОН · ПАУЗА" else "ЭКРОН"
                if(canContinue){
                    menu.addView(menuButton("Продолжить"){if(gameReady)resumeGame() else message("Карта готовится в фоне")})
                }else{
                    menu.addView(menuButton("Начать игру"){page=1;buildMenu()})
                }
                menu.addView(menuButton("Новая игра"){page=1;buildMenu()})
                if(canContinue)menu.addView(menuButton("Сохранить игру"){if(gameReady)saveGame();notice="Сохранение…";buildMenu()})
                menu.addView(menuButton("Настройки"){page=2;buildMenu()})
            }
            1->{
                titleText.text="ВЫБОР РЕЖИМА"
                menu.addView(menuButton("Творческий · играть"){if(gameReady){canContinue=true;newGame()}else message("Карта готовится в фоне")})
                menu.addView(menuButton("Назад"){page=0;buildMenu()})
            }
            2->{
                titleText.text="НАСТРОЙКИ"
                menu.addView(menuButton(if(muted)"Звук: выключен" else "Звук: включён"){muted=!muted;(context as? MainActivity)?.setLobbyMuted(muted);buildMenu()})
                menu.addView(menuButton("Управление: игрок"){(context as? MainActivity)?.editControls("player")})
                menu.addView(menuButton("Управление: машина"){(context as? MainActivity)?.editControls("car")})
                menu.addView(menuButton("Управление: дрон"){(context as? MainActivity)?.editControls("drone")})
                menu.addView(menuButton("Графика: ${graphicsLabel()}"){page=3;buildMenu()})
                menu.addView(menuButton("Назад"){page=0;buildMenu()})
            }
            3->{
                titleText.text="ГРАФИКА"
                val prefs=context.getSharedPreferences("graphics",0)
                val quality=prefs.getInt("quality",1).coerceIn(0,2)
                val fps=prefs.getInt("fps",60)
                menu.addView(menuButton("Качество: ${listOf("Среднее","Высокое","Ультра")[quality]}"){ (context as? MainActivity)?.setGraphics((quality+1)%3,null);frameLimit=context.getSharedPreferences("graphics",0).getInt("fps",60);buildMenu() })
                menu.addView(menuButton("FPS: $fps"){ (context as? MainActivity)?.setGraphics(null,if(fps==60)30 else 60);frameLimit=context.getSharedPreferences("graphics",0).getInt("fps",60);buildMenu() })
                menu.addView(menuButton("Назад"){page=2;buildMenu()})
            }
        }
    }

    /** Use the very same saved player layout as the actual game HUD. */
    fun syncPlayerControls(root:ViewGroup,walk:View,jump:View,crouch:View){
        val prefs=context.getSharedPreferences("controls-player",0)
        fun find(group:ViewGroup,target:View,path:String="",ox:Int=0,oy:Int=0):Triple<String,Int,Int>?{
            for(i in 0 until group.childCount){val v=group.getChildAt(i);val key="$path/$i"
                if(v===target)return Triple(key,ox+v.left,oy+v.top)
                if(v is ViewGroup){val found=find(v,target,key,ox+v.left,oy+v.top);if(found!=null)return found}
            };return null
        }
        for((source,dest) in listOf(walk to walkControl,jump to jumpControl,crouch to crouchControl)){
            val info=find(root,source)?:continue;if(source.width<1||source.height<1)continue
            dest.layoutParams=LayoutParams(source.width,source.height,Gravity.TOP or Gravity.START).apply { leftMargin=info.second;topMargin=info.third }
            dest.translationX=prefs.getFloat("${info.first}.x",0f)*root.width;dest.translationY=prefs.getFloat("${info.first}.y",0f)*root.height
            val scale=prefs.getFloat("${info.first}.s",1f);dest.scaleX=scale;dest.scaleY=scale
        }
    }
    private var last=0L
    private val paint=Paint(Paint.ANTI_ALIAS_FLAG or Paint.FILTER_BITMAP_FLAG)
    private val screens=Array(4){Bitmap.createBitmap(768,432,Bitmap.Config.ARGB_8888)}
    private var dirty=true
    private var videoProjected=false
    private data class P(val x:Float,val y:Float,val z:Float)
    fun setMuted(value:Boolean){muted=value;intro?.setMuted(value);dirty=true}
    fun message(text:String){notice=text;dirty=true;if(::noticeText.isInitialized)noticeText.text=text;invalidate()}
    fun refresh(){dirty=true;last=0;if(::menu.isInitialized)buildMenu();invalidate()}
    private fun project(p:P):PointF?{
        val dx=p.x-posX;val dz=p.z-posZ;val dy=p.y-eye
        val qx=cos(yaw)*dx-sin(yaw)*dz
        val qz=sin(yaw)*dx+cos(yaw)*dz
        val qy=cos(pitch)*dy+sin(pitch)*qz
        val depth=sin(pitch)*dy-cos(pitch)*qz
        if(depth<.08f)return null
        val f=height*.85f
        return PointF(width/2f+qx*f/depth,height/2f-qy*f/depth)
    }
    private fun quad(c:Canvas,points:List<P>,color:Int,bitmap:Bitmap?=null,index:Int=-1){
        val p=points.map { project(it)?:return }
        val path=Path().apply { moveTo(p[0].x,p[0].y);for(i in 1..3)lineTo(p[i].x,p[i].y);close() }
        paint.color=color;c.drawPath(path,paint)
        if(bitmap!=null){
            val src=floatArrayOf(0f,0f,768f,0f,768f,432f,0f,432f)
            val dst=p.flatMap { listOf(it.x,it.y) }.toFloatArray()
            val m=Matrix();m.setPolyToPoly(src,0,dst,0,4)
            c.save();c.clipPath(path);paint.color=Color.WHITE;c.drawBitmap(bitmap,m,paint);c.restore()

            if(index==2&&width>0&&height>0){
                val video=Matrix();video.setPolyToPoly(floatArrayOf(0f,0f,width.toFloat(),0f,width.toFloat(),height.toFloat(),0f,height.toFloat()),0,dst,0,4)
                intro?.setTransform(video);videoProjected=true
            }
        }
    }
    private fun rebuild(){
        for(i in 0..3){val c=Canvas(screens[i]);c.drawColor(0xff14251f.toInt())
            paint.color=0xff92dbad.toInt();paint.textSize=36f;paint.typeface=Typeface.create(Typeface.MONOSPACE,Typeface.BOLD)
            c.drawText(listOf("ЭКРОН · КОМАНДНЫЙ ПУНКТ","НАСТРОЙКИ / ИНФОРМАЦИЯ","ИНТРО","ПОЛЕВОЕ РУКОВОДСТВО")[i],24f,52f,paint)
            paint.textSize=25f
            val lines=when(i){
                0 -> listOf("Основное меню — кнопки справа на экране телефона")
                1 -> listOf("Настройки: звук, управление, графика")
                2 -> if(introError)listOf("Не удалось загрузить интро") else listOf("Интро · коснитесь экрана: пауза")
                else -> listOf("Руководство: ходите стрелкой, смотрите справа")
            }
            paint.textSize=25f
            lines.forEachIndexed { n,text ->
                paint.color=0xffbcebd0.toInt();c.drawText(text.take(47),24f,120f+n*62f,paint)
            }
        };dirty=false
    }
    override fun onDraw(c:Canvas){
        val now=System.nanoTime();val dt=if(last==0L)0f else ((now-last)/1e9f).coerceIn(0f,.04f);last=now
        val norm=max(1f,hypot(right,forward));posX+=((cos(yaw)*right-sin(yaw)*forward)/norm)*2.7f*dt;posZ+=((-sin(yaw)*right-cos(yaw)*forward)/norm)*2.7f*dt
        posX=posX.coerceIn(-7.4f,7.4f);posZ=posZ.coerceIn(-7.4f,7.4f)
        vertical-=18f*dt;jump=max(0f,jump+vertical*dt);if(jump==0f)vertical=0f;eye=(if(crouch)1f else 1.7f)+jump
        if(dirty)rebuild();c.drawColor(0xff18211e.toInt())
        // Tile-sized quads clip against the near plane without dropping a whole wall.
        for(a in -8..7)for(b in -8..7){
            val u=a.toFloat();val v=b.toFloat();val shade=if((a+b)%2==0)0xff30372f.toInt() else 0xff343c32.toInt()
            quad(c,listOf(P(u,0f,v),P(u+1,0f,v),P(u+1,0f,v+1),P(u,0f,v+1)),shade)
        }
        for(wall in 0..3)for(a in -8..7)for(h in 0..4){
            fun wallPoint(u:Float,y:Float):P=when(wall){0->P(u,y,-8f);1->P(8f,y,u);2->P(-u,y,8f);else->P(-8f,y,-u)}
            quad(c,listOf(wallPoint(a.toFloat(),h.toFloat()),wallPoint(a+1f,h.toFloat()),wallPoint(a+1f,h+1f),wallPoint(a.toFloat(),h+1f)),if(h==0)0xff414c35.toInt() else 0xff242e28.toInt())
        }
        // Far-to-near order, room geometry is behind every monitor.
        val ids=(0..3).sortedByDescending { i->val p=when(i){0->P(0f,2.5f,-7.95f);1->P(7.95f,2.5f,0f);2->P(0f,2.5f,7.95f);else->P(-7.95f,2.5f,0f)};(p.x-posX).pow(2)+(p.z-posZ).pow(2) }
        for(i in ids){fun point(u:Float,y:Float)=when(i){0->P(u,y,-7.94f);1->P(7.94f,y,u);2->P(-u,y,7.94f);else->P(-7.94f,y,-u)}
            quad(c,listOf(point(-3.2f,3.9f),point(3.2f,3.9f),point(3.2f,.3f),point(-3.2f,.3f)),Color.BLACK,screens[i],i)
        }
        intro?.screenVisible=videoProjected
        postInvalidateDelayed(if(frameLimit==30)33L else 16L)
    }
    private fun graphicsLabel():String{
        val p=context.getSharedPreferences("graphics",0)
        return "${listOf("Среднее","Высокое","Ультра")[p.getInt("quality",1).coerceIn(0,2)]} · ${p.getInt("fps",60)} FPS"
    }
    override fun onVisibilityChanged(changedView:View,visibility:Int){super.onVisibilityChanged(changedView,visibility);intro?.playing=isShown&&!introPaused}
    override fun onWindowVisibilityChanged(visibility:Int){super.onWindowVisibilityChanged(visibility);intro?.playing=visibility==VISIBLE&&!introPaused}
    override fun onDetachedFromWindow(){intro?.close();intro=null;super.onDetachedFromWindow();last=0;walkControl.reset();lookControl.reset()}
}
