package com.example.realms

import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.FrameLayout
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith

/** Device-side coverage; compiling this is not a substitute for running it. */
@RunWith(AndroidJUnit4::class)
class LobbyControlsTest {
    private fun children(root:ViewGroup):List<View> = (0 until root.childCount).flatMap {
        val v=root.getChildAt(it);listOf(v)+(if(v is ViewGroup)children(v) else emptyList())
    }
    private fun layout(v:View,w:Int,h:Int){v.measure(View.MeasureSpec.makeMeasureSpec(w,View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(h,View.MeasureSpec.EXACTLY));v.layout(0,0,w,h)}
    @Test fun lobbyUsesActualPlayerControlWidgets(){
        val instrumentation=InstrumentationRegistry.getInstrumentation()
        instrumentation.runOnMainSync {
            val room=LobbyView(instrumentation.targetContext,{},{},{})
            val controls=children(room)
            assertEquals(1,controls.count { it is StickView })
            assertEquals(1,controls.count { it is LookView })
            assertEquals(setOf("jump","crouch"),controls.filterIsInstance<HudButton>().map { it.symbol }.toSet())
        }
    }
    @Test fun editorHasLargeSeparateActionsAndExitSaves(){
        val instrumentation=InstrumentationRegistry.getInstrumentation()
        instrumentation.runOnMainSync {
            val context=instrumentation.targetContext
            val d=context.resources.displayMetrics.density
            val root=FrameLayout(context)
            root.addView(HudButton(context,"Прыжок","jump"),FrameLayout.LayoutParams((64*d).toInt(),(64*d).toInt()))
            layout(root,(800*d).toInt(),(360*d).toInt())
            var exited=false
            val editor=ControlEditorView(context,root,"test-ui"){exited=true}
            layout(editor,root.width,root.height)
            val buttons=children(editor).filterIsInstance<Button>()
            assertEquals(5,buttons.size)
            for(button in buttons)assertTrue("Action must have a >=48dp hit area",button.height>=48*d)
            assertEquals(2,buttons.count { it.text.contains("Меньше")||it.text.contains("Больше") })
            val exit=buttons.single { it.text=="Сохранить" }
            exit.performClick();assertTrue(exited)
            assertTrue(context.getSharedPreferences("controls-test-ui",0).contains("/0.s"))
            context.getSharedPreferences("controls-test-ui",0).edit().clear().commit()
        }
    }
}
