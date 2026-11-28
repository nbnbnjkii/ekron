package com.example.realms

import android.graphics.*

/** Native monitor artwork adapted from the supplied military lobby design.
 * Screen 02 (settings) is deliberately excluded and keeps its existing renderer. */
internal object LobbyMonitorArt {
    fun draw(c:Canvas,screen:Int,page:Int,ready:Boolean,canContinue:Boolean,notice:String,introError:Boolean){
        require(screen!=1)
        val p=Paint(Paint.ANTI_ALIAS_FLAG)
        val accent=0xffa7eac0.toInt()
        p.shader=LinearGradient(0f,0f,0f,432f,0xff0a1a12.toInt(),0xff112a1d.toInt(),Shader.TileMode.CLAMP)
        c.drawRect(0f,0f,768f,432f,p);p.shader=null
        p.color=0x1078dca0
        for(x in 0..768 step 32)c.drawLine(x.toFloat(),0f,x.toFloat(),432f,p)
        for(y in 0..432 step 32)c.drawLine(0f,y.toFloat(),768f,y.toFloat(),p)
        val star=Path()
        for(i in 0 until 10){val a=-Math.PI/2+i*Math.PI/5;val r=if(i%2==0)155f else 63f;val x=620f+kotlin.math.cos(a).toFloat()*r;val y=250f+kotlin.math.sin(a).toFloat()*r;if(i==0)star.moveTo(x,y)else star.lineTo(x,y)}
        star.close();p.color=0x0ca7eac0;c.drawPath(star,p)
        p.shader=LinearGradient(0f,0f,768f,0f,0xff22563a.toInt(),0xff123022.toInt(),Shader.TileMode.CLAMP)
        c.drawRect(0f,0f,768f,62f,p);p.shader=null;p.color=accent;c.drawRect(0f,60f,768f,62f,p)
        p.style=Paint.Style.STROKE;p.strokeWidth=1.5f;c.drawRect(14f,12f,54f,50f,p);p.style=Paint.Style.FILL
        p.typeface=Typeface.create(Typeface.MONOSPACE,Typeface.BOLD);p.textSize=22f;c.drawText("0${screen+1}",20f,39f,p)
        p.textSize=27f;c.drawText(when(screen){0->"КОМАНДНЫЙ ПУНКТ";2->"ИНТРО";else->"ПОЛЕВОЕ РУКОВОДСТВО"},68f,42f,p)
        p.textSize=14f;p.color=0xff70a284.toInt();p.textAlign=Paint.Align.RIGHT
        c.drawText("УЗЕЛ 01",750f,24f,p);c.drawText("КАНАЛ 0${screen+1}/04",750f,46f,p);p.textAlign=Paint.Align.LEFT
        fun row(n:Int,text:String,button:Boolean=false,enabled:Boolean=true,title:Boolean=false){
            val y=80f+n*64
            if(button){
                p.shader=LinearGradient(12f,0f,756f,0f,if(enabled)0xff1e402e.toInt() else 0xff13241b.toInt(),if(enabled)0xff2a4f3a.toInt() else 0xff0f1d16.toInt(),Shader.TileMode.CLAMP)
                c.drawRoundRect(RectF(12f,y,756f,y+58),8f,8f,p);p.shader=null
                p.style=Paint.Style.STROKE;p.strokeWidth=1.3f;p.color=0x48a7eac0;c.drawRoundRect(RectF(12f,y,756f,y+58),8f,8f,p);p.style=Paint.Style.FILL
                if(enabled){p.color=0xff6fcf97.toInt();c.drawRoundRect(RectF(17f,y+11,21f,y+47),2f,2f,p);p.textSize=30f;c.drawText("›",722f,y+38,p)}
            }
            p.color=if(!enabled)0xff58806a.toInt() else if(title)accent else Color.WHITE
            p.textSize=if(title)30f else 25f;c.drawText(text.take(46),if(button)36f else 24f,y+38,p)
            if(title){p.color=0x70a7eac0;c.drawRect(24f,y+48,144f,y+51,p)}
        }
        when(screen){
            0->if(page==0){
                row(0,"ДОБРО ПОЖАЛОВАТЬ",title=true)
                row(1,if(canContinue)"Продолжить игру" else "Начать игру",true,ready)
                row(2,"Новая игра",true,ready)
                row(3,if(canContinue)"Сохранить игру" else "Выберите режим игры",canContinue,ready&&canContinue)
                row(4,if(!ready)"Карта готовится в фоне…" else notice)
            }else{
                row(0,"ВЫБЕРИТЕ РЕЖИМ",title=true);row(1,"Творческий · играть",true,ready)
                row(2,"Выживание пока недоступно",enabled=false);row(3,"Назад",true);row(4,notice)
            }
            2->{row(0,"ВИДЕОКАНАЛ",title=true);row(2,if(introError)"Не удалось открыть видео" else "Касание: пауза / продолжить")}
            3->{row(0,"ПОЛЕВОЙ ИНСТРУКТАЖ",title=true);row(1,"Левый стик — движение");row(2,"Проведение справа — обзор");row(3,"Касайтесь кнопок на мониторах");row(4,"Настройки — на правой стене")}
        }
        p.color=0x66000000;c.drawRect(0f,404f,768f,432f,p);p.color=0x59a7eac0;c.drawRect(0f,404f,768f,405f,p)
        p.textSize=14f;c.drawText("ЭКРОН · ЛОББИ · УЗЕЛ 01",14f,423f,p)
        p.textAlign=Paint.Align.RIGHT;c.drawText(if(ready)"КАРТА ГОТОВА" else "ПОДГОТОВКА КАРТЫ",750f,423f,p)
        p.color=0x0e000000;for(y in 0..432 step 3)c.drawRect(0f,y.toFloat(),768f,y+1f,p)
    }
}
