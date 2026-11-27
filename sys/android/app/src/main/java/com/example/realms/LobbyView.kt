package com.example.realms

import android.content.Context
import android.graphics.*
import android.view.*
import android.widget.FrameLayout
import java.nio.ByteBuffer

/** Only touch widgets and monitor artwork live here. The room/camera/physics are GLES-native. */
class LobbyView(context:Context,private val resumeGame:()->Unit,private val newGame:()->Unit,private val saveGame:()->Unit):FrameLayout(context){
    private external fun nativeInput(right:Float,forward:Float,dx:Float,dy:Float,jump:Boolean,crouch:Boolean)
    private external fun nativeScreen(index:Int,pixels:ByteArray)
    private external fun nativeHit(x:Float,y:Float,aspect:Float):FloatArray
    private external fun nativeVideoCorners(aspect:Float):FloatArray
    var canContinue=false
        set(value){if(field!=value){field=value;dirty=true;invalidate()}}
    var gameReady=false
        set(value){if(field!=value){field=value;dirty=true;invalidate()}}
    private var page=0
    private var graphicsPage=false
    private var muted=context.getSharedPreferences("settings",0).getBoolean("muted",false)
    private var notice=""
    private var dirty=true
    private var right=0f;private var forward=0f;private var crouched=false
    private var introPaused=false
    private var introError=false
    private var intro:LobbyIntro?=null
    private val walkControl=StickView(context){r,f->right=r;forward=f;input()}
    private val lookControl=LookView(context){dx,dy->input(dx,dy)}
    private val jumpControl=HudButton(context,"Прыжок","jump")
    private val crouchControl=HudButton(context,"Присесть","crouch")
    private val pixels=Bitmap.createBitmap(768,432,Bitmap.Config.ARGB_8888)
    private val bytes=ByteBuffer.allocate(768*432*4)
    private val paint=Paint(Paint.ANTI_ALIAS_FLAG or Paint.FILTER_BITMAP_FLAG)
    private fun input(dx:Float=0f,dy:Float=0f,jump:Boolean=false){nativeInput(right,forward,dx,dy,jump,crouched)}
    init {
        setWillNotDraw(false);isMotionEventSplittingEnabled=true
        intro=LobbyIntro(context){introError=true;dirty=true;invalidate()}
        addView(intro,LayoutParams(-1,-1))
        lookControl.onTap={x,y->click(x,y)};addView(lookControl,LayoutParams(-1,-1))
        val d=resources.displayMetrics.density
        fun dp(n:Int)=(n*d).toInt()
        val stick=(resources.configuration.screenHeightDp*.32f).toInt().coerceIn(120,184)
        val size=(resources.configuration.screenHeightDp*.145f).toInt().coerceIn(60,64)
        addView(walkControl,LayoutParams(dp(stick),dp(stick),Gravity.BOTTOM or Gravity.START).apply { leftMargin=dp(14);bottomMargin=dp(22) })
        jumpControl.setOnClickListener { input(jump=true) }
        jumpControl.setOnTouchListener { v,e->when(e.actionMasked){MotionEvent.ACTION_DOWN->{v.isPressed=true;v.performClick()};MotionEvent.ACTION_UP,MotionEvent.ACTION_CANCEL->v.isPressed=false};true }
        crouchControl.setOnClickListener { crouched=!crouched;crouchControl.isSelected=crouched;input() }
        addView(jumpControl,LayoutParams(dp(size),dp(size),Gravity.BOTTOM or Gravity.END).apply { rightMargin=dp(8);bottomMargin=dp(size+8) })
        addView(crouchControl,LayoutParams(dp(size),dp(size),Gravity.BOTTOM or Gravity.END).apply { rightMargin=dp(8);bottomMargin=dp(8) })
    }
    fun message(text:String){if(notice!=text){notice=text;dirty=true;invalidate()}}
    fun refresh(){dirty=true;invalidate()}
    fun back(){page=0;graphicsPage=false;refresh()}
    fun setMuted(value:Boolean){muted=value;intro?.setMuted(value);refresh()}
    private fun artwork(){
        val prefs=context.getSharedPreferences("graphics",0)
        for(i in 0..3){
            val c=Canvas(pixels);c.drawColor(0xff13281e.toInt())
            paint.typeface=Typeface.create(Typeface.MONOSPACE,Typeface.BOLD);paint.color=0xffa7eac0.toInt();paint.textSize=32f
            c.drawText(listOf("ЭКРОН · КОМАНДНЫЙ ПУНКТ","НАСТРОЙКИ / ИНФОРМАЦИЯ","ИНТРО · касание: пауза","ПОЛЕВОЕ РУКОВОДСТВО")[i],20f,48f,paint)
            fun row(n:Int,text:String,button:Boolean=true,left:Float=12f,end:Float=756f,active:Boolean=false){
                val y=80f+n*64
                if(button){paint.color=if(active)0xff477d60.toInt() else 0xff294c39.toInt();c.drawRoundRect(RectF(left,y,end,y+58),8f,8f,paint)}
                paint.color=Color.WHITE;paint.textSize=27f;c.drawText(text.take(47),left+12,y+38,paint)
            }
            when(i){
                0->if(page==0){
                    row(0,"Добро пожаловать",false)
                    row(1,if(canContinue)"Продолжить игру" else "Начать игру")
                    row(2,"Новая игра")
                    row(3,if(canContinue)"Сохранить игру" else "Выберите режим игры",canContinue)
                    row(4,if(!gameReady)"Карта готовится в фоне…" else notice,false)
                }else{
                    row(0,"Выберите режим игры",false);row(1,"Творческий · играть");row(2,"Выживание пока недоступно",false);row(3,"Назад");row(4,notice,false)
                }
                1->if(!graphicsPage){
                    row(0,if(muted)"Звук: выключен" else "Звук: включён");row(1,"Управление: игрок");row(2,"Управление: машина");row(3,"Управление: дрон");row(4,"Настройки графики")
                }else{
                    for(q in 0..2)row(0,listOf("Средние","Высокие","Ультра")[q],true,q*256f+8,(q+1)*256f-8,prefs.getInt("quality",1)==q)
                    row(1,"30 FPS",true,8f,376f,prefs.getInt("fps",60)==30);row(1,"60 FPS",true,392f,760f,prefs.getInt("fps",60)==60)
                    row(2,"Назад");row(3,"Разрешение рендера: 60 / 80 / 100%",false)
                }
                2->{row(1,if(introError)"Не удалось открыть видео" else "Коснитесь экрана: пауза / продолжить",false)}
                3->{row(0,"Здесь нет оружия и строительства",false);row(1,"Обзор и ходьба — как в игре",false);row(2,"Кнопки находятся на стенах",false);row(3,"Продолжить возвращает в сессию",false);row(4,"Настройки — на правой стене",false)}
            }
            bytes.clear();pixels.copyPixelsToBuffer(bytes);nativeScreen(i,bytes.array())
        };dirty=false
    }
    private fun click(x:Float,y:Float){
        if(width<=0||height<=0)return
        val hit=nativeHit(x/width,y/height,width.toFloat()/height)
        if(hit.size!=3||hit[0]<0)return
        val screen=hit[0].toInt();val u=hit[1]*768;val v=hit[2]*432
        if(screen==2){introPaused=!introPaused;intro?.playing=!introPaused;return}
        if(v<80||v>400)return
        val row=((v-80)/64).toInt()
        if(screen==0){
            if(row==3&&page==1){page=0;refresh();return}
            if(!gameReady){message("Карта готовится в фоне…");return}
            if(page==0)when(row){1->if(canContinue)resumeGame()else{page=1;refresh()};2->{page=1;refresh()};3->if(canContinue)saveGame()}
            else if(row==1)newGame()
        }
        if(screen==1){
            val host=context as? MainActivity
            if(graphicsPage)when(row){0->host?.setGraphics((u/256).toInt().coerceIn(0,2),null);1->host?.setGraphics(null,if(u<384)30 else 60);2->graphicsPage=false}
            else when(row){0->host?.setLobbyMuted(!muted);1->host?.editControls("player");2->host?.editControls("car");3->host?.editControls("drone");4->graphicsPage=true}
            refresh()
        }
    }
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
    override fun onDraw(canvas:Canvas){
        // No room/camera projection here: only upload changed monitor artwork.
        if(dirty)artwork()
        if(width>0&&height>0){val p=nativeVideoCorners(width.toFloat()/height)
            intro?.screenVisible=p.size==8
            if(p.size==8){for(i in p.indices)p[i]*=if(i%2==0)width else height
                val m=Matrix();m.setPolyToPoly(floatArrayOf(0f,0f,width.toFloat(),0f,width.toFloat(),height.toFloat(),0f,height.toFloat()),0,p,0,4);intro?.setTransform(m)}
        }
        postInvalidateDelayed(if(context.getSharedPreferences("graphics",0).getInt("fps",60)==30)33L else 16L)
    }
    override fun onVisibilityChanged(changedView:View,visibility:Int){super.onVisibilityChanged(changedView,visibility);if(visibility!=VISIBLE){right=0f;forward=0f;nativeInput(0f,0f,0f,0f,false,false)};intro?.playing=isShown&&!introPaused}
    override fun onWindowVisibilityChanged(visibility:Int){super.onWindowVisibilityChanged(visibility);intro?.playing=visibility==VISIBLE&&!introPaused}
    override fun onDetachedFromWindow(){nativeInput(0f,0f,0f,0f,false,false);intro?.close();super.onDetachedFromWindow()}
}
