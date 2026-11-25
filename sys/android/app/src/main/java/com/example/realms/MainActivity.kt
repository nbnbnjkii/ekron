package com.example.realms

import android.app.AlertDialog
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import java.util.concurrent.Executors
import android.graphics.drawable.GradientDrawable
import android.media.MediaPlayer
import android.view.Surface
import android.view.TextureView
import android.graphics.SurfaceTexture
import android.os.SystemClock
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.content.ClipData
import android.content.ClipboardManager
import android.graphics.Color
import android.view.Gravity
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.*
import java.util.Locale
import com.google.androidgamesdk.GameActivity

/** Android HUD over the native GLES scene; loading remains visible until its first frame. */
class MainActivity : GameActivity() {
    companion object { init { System.loadLibrary("realms") } }
    private external fun nativeReady(ready: Boolean)
    private var player: MediaPlayer? = null
    private var video: TextureView? = null
    private var videoEndedAt = Long.MAX_VALUE
    private var active = false
    private var introFinished = false
    private external fun nativeCommand(command: Int)
    private external fun nativeStatus(): FloatArray
    private external fun nativeVertical(value: Float)
    private external fun nativeDroneInput(throttle:Float,yaw:Float,pitch:Float,roll:Float)
    private external fun nativeMove(right: Float, forward: Float)
    private external fun nativeDriveSteer(steer: Float)
    private external fun nativeDriveThrottle(throttle: Float)
    private external fun nativeLook(dx: Float, dy: Float)
    private lateinit var lookArea: LookView
    private lateinit var fpvLayer: FrameLayout
    private lateinit var fpvControls: FpvControlView
    private lateinit var dronePlace: HudButton
    private lateinit var remoteActions: LinearLayout
    private var droneState=0
    private val normalHudVisibility=mutableMapOf<View,Int>()
    private var paused = false
    private var worldReady = false
    private var flyingNow = false
    private lateinit var joystick: StickView
    private lateinit var driveStick: StickView
    private lateinit var movementControls: LinearLayout
    private var wasDriving = false
    private val toolsState = ToolState()
    private var requestedTool: HandTool? = null
    private lateinit var toolActions: LinearLayout
    private var interactionCode=0
    private lateinit var interactionControl: HudButton
    private lateinit var catalogControl: HudButton
    private lateinit var spawnActions: LinearLayout
    private lateinit var gravityActions: LinearLayout
    private lateinit var removeControl: HudButton
    private lateinit var freezeControl: HudButton
    private lateinit var grabControl: HudButton
    private lateinit var rotateHeldControl: HudButton
    private lateinit var crouchControl: HudButton
    private lateinit var proneControl: HudButton
    private lateinit var viewToggle: HudButton
    private val items = BuildCatalog.entries.map { it.label }.toTypedArray()
    private val catalogImages=arrayOfNulls<Bitmap>(BuildCatalog.entries.size)
    private val catalogExecutor=Executors.newSingleThreadExecutor()
    private var catalogAdapter: BaseAdapter? = null
    private lateinit var hud: FrameLayout
    private lateinit var toolWheel: ToolWheelView
    private lateinit var weapon: ToolArtView
    private lateinit var selectedImage: ImageView
    private lateinit var selectedSlot: FrameLayout
    private lateinit var selectedSwatch: MaterialBlockView
    private var popup: View? = null
    private var pendingSelection: Int? = null
    private var pendingSurface: Int? = null
    private var selectedSurface=1
    private val materialImages=arrayOfNulls<Bitmap>(7)
    private val materialIds=intArrayOf(5,1,2,3)
    private val materialNames=arrayOf("Дерево","Бетон","Кирпич","Металл")
    private var selected = 0
    private val handler = Handler(Looper.getMainLooper())
    private lateinit var coordinates: TextView
    private lateinit var flight: Button
    private lateinit var altitude: LinearLayout
    private lateinit var loading: LinearLayout
    private lateinit var loadingText: TextView
    private lateinit var spinner: ProgressBar
    private lateinit var notice: TextView
    private var latest = floatArrayOf(0f, 1.7f, 0f)
    private val poll = object : Runnable {
        override fun run() {
            val state = nativeStatus()
            if (state.size >= 6) {
                if(state.size >= 8) {
                    val confirmed=HandTool.values().getOrElse(state[7].toInt()) { HandTool.HAND_EMPTY }
                    if(requestedTool==confirmed)requestedTool=null
                    if(requestedTool==null&&confirmed!=toolsState.tool)toolsState.select(confirmed)
                    if(confirmed==toolsState.tool)toolsState.confirmHolding(state[6]>.5f)
                    if(state.size>=11 && state[4].toInt()==6){
                        val nativeType=state[9].toInt();val nativeSurface=state[10].toInt()
                        val nativeIndex=BuildCatalog.entries.indexOfFirst { it.nativeType==nativeType }
                        if(pendingSelection==nativeIndex)pendingSelection=null
                        if(pendingSurface==nativeSurface)pendingSurface=null
                        if(pendingSelection==null && nativeIndex in items.indices){selected=nativeIndex;selectedSurface=BuildCatalog.entries[selected].defaultSurface}
                        if(pendingSurface==null && nativeSurface in listOf(1,2,3,5,6))selectedSurface=nativeSurface
                    }
                    if(state.size>=12){interactionCode=state[11].toInt();interactionControl.visibility=if(interactionCode>0)View.VISIBLE else View.GONE
                        interactionControl.caption=when(interactionCode){6->"Подключиться";1->"Открыть";2->"Закрыть";3->"Сесть";4->"Выйти";else->""}
                        interactionControl.contentDescription=interactionControl.caption;interactionControl.invalidate()
                        if(::driveStick.isInitialized){val driving=interactionCode==4;driveStick.visibility=if(driving)View.VISIBLE else View.GONE
                            if(!driving){driveStick.reset();if(wasDriving){joystick.reset();nativeDriveThrottle(0f)}}
                            wasDriving=driving
                            // Keep the exit button clear of the drive stick area.
                            val stick=(resources.configuration.screenHeightDp*.32f).toInt().coerceIn(120,184)
                            val lp=interactionControl.layoutParams as FrameLayout.LayoutParams
                            val margin=dp(if(driving)stick+30 else 8)
                            if(lp.bottomMargin!=margin){lp.bottomMargin=margin;interactionControl.layoutParams=lp}
                        }
                    }
                    if(state.size>=13){freezeControl.caption=if(state[12]>.5f) "Разморозить" else "Заморозить";freezeControl.contentDescription=freezeControl.caption;freezeControl.invalidate()}
                    if(state.size>=15 && ::viewToggle.isInitialized){ viewToggle.isSelected=state[14]>.5f }
                    renderTools()
                }
                latest = state.copyOfRange(0, 3)
                coordinates.text = String.format(Locale.US, "X: %.1f   Y: %.1f   Z: %.1f", state[0], state[1], state[2])
                val flying = state[3] > 0.5f
                flyingNow = flying
                flight.text = if (flying) "✈ Полёт: ВКЛ" else "✈ Полёт: ВЫКЛ"
                altitude.visibility = if (flying) View.VISIBLE else View.GONE
                val stage = state[4].toInt()
                worldReady = stage == 6 && introFinished
                val unlocked = worldReady && active
                nativeReady(unlocked && !paused)
                weapon.running=unlocked && !paused
                loading.visibility = if (unlocked) View.GONE else View.VISIBLE
                spinner.visibility = if (stage < 0) View.GONE else View.VISIBLE
                if(state.size>=9)spinner.progress=state[8].toInt().coerceIn(0,100)
                loadingText.text = when (stage) {
                    0 -> "Подготовка графики…"
                    1 -> "Загрузка текстур…"
                    2 -> when {
                        state.size < 9 || state[8] < 21 -> "Новая карта: чтение GLB…"
                        state[8] < 37 -> "Новая карта: геометрия и материалы…"
                        state[8] < 42 -> "Новая карта: стены и границы…"
                        state[8] < 49 -> "Новая карта: поиск старта…"
                        else -> "Новая карта: построение коллизий…"
                    }
                    3 -> "Подготовка УАЗа и парковки…"
                    4 -> "Загрузка моделей на GPU…"
                    5 -> "Подготовка первого кадра…"
                    6 -> "Мир готов"
                    else -> "Не удалось запустить рендерер.\nПерезапусти игру. Ошибка записана в logcat / Ekron."
                }
                if(stage in 0..5 && state.size>=9)loadingText.append(" ${state[8].toInt()}%")
                notice.text = when (state[5].toInt()) {
                    1 -> "Точка из конфига небезопасна — спавн перенесён на открытое место."
                    2 -> "Безопасная точка пока недоступна — освободите спавн."
                    3 -> "Нет свободного места для УАЗа. Освободите парковку."
                    5 -> "Нет безопасного места рядом для выхода."
                    4 -> "Игрок возвращён на безопасный спавн."
                    else -> if (flying) "Сквозь стены · ↑ / ↓ — высота" else when(toolsState.tool) {
                        HandTool.HAND_EMPTY -> "Пустые руки"
                        HandTool.SPAWN_GUN -> "Спавн-пушка: ${items[selected]}"
                        HandTool.GRAVITY_GUN -> if(toolsState.holding) "Предмет захвачен" else "Грави-пушка: наведите на свою постройку"
                        HandTool.DELETE_TOOL -> "Удаление своих построек"
                        HandTool.DRONE_REMOTE -> "Пульт FPV · подключение к дрону"
                    }
                }
            }
            if(state.size>=14 && ::fpvLayer.isInitialized){
                droneState=state[13].toInt()
                dronePlace.visibility=if(droneState==1)View.VISIBLE else View.GONE
                val fpv=droneState in 3..5
                if(fpv){
                    if(normalHudVisibility.isEmpty()){
                        closePopup();toolWheel.cancel();joystick.reset();lookArea.reset()
                        for(i in 0 until hud.childCount){val child=hud.getChildAt(i);if(child!==fpvLayer)normalHudVisibility[child]=child.visibility}
                        Toast.makeText(this@MainActivity,"Слева — высота и рыскание. Справа — наклоны. × — к пульту.",Toast.LENGTH_LONG).show()
                    }
                    for(i in 0 until hud.childCount){val child=hud.getChildAt(i);if(child!==fpvLayer)child.visibility=View.INVISIBLE}
                    fpvLayer.visibility=View.VISIBLE;fpvLayer.bringToFront()
                }else {
                    fpvLayer.visibility=View.GONE
                    if(normalHudVisibility.isNotEmpty()){
                        fpvControls.reset();normalHudVisibility.forEach { (view,visibility)->view.visibility=visibility };normalHudVisibility.clear();renderTools()
                        dronePlace.visibility=if(droneState==1)View.VISIBLE else View.GONE
                    }
                }
            }
            if (!introFinished && videoEndedAt != Long.MAX_VALUE && SystemClock.elapsedRealtime() - videoEndedAt >= 3000) introFinished = true
            handler.postDelayed(this, 100)
        }
    }
    private fun dp(value: Int) = (value * resources.displayMetrics.density).toInt()
    private fun label(text: String, size: Float) = TextView(this).apply {
        this.text = text; textSize = size; setTextColor(Color.WHITE)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        collectExitReport()
        window.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val overlay = FrameLayout(this).apply { isMotionEventSplittingEnabled = true }
        hud=overlay
        lookArea=LookView(this) { dx,dy -> nativeLook(dx,dy) }
        overlay.addView(lookArea,FrameLayout.LayoutParams(-1,-1))
        weapon=ToolArtView(this,true).apply { visibility=View.GONE }
        overlay.addView(weapon,FrameLayout.LayoutParams(dp(200),dp(125),Gravity.BOTTOM or Gravity.END).apply { rightMargin=dp(125);bottomMargin=dp(78) })
        val heading = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(12), dp(8), dp(12), dp(8))
            background = GradientDrawable().apply { setColor(0xaa263039.toInt()); cornerRadius = dp(5).toFloat() }
        }
        heading.addView(label("ЭКРОН • ПЕСОЧНИЦА", 14f))
        coordinates = label("X: —   Y: —   Z: —", 13f).apply {
            contentDescription = "Координаты игрока. Нажмите, чтобы скопировать для конфига"
            setOnClickListener {
                val json = String.format(Locale.US, "{\"enabled\":true,\"position\":{\"x\":%.4f,\"y\":%.4f,\"z\":%.4f},\"yawDegrees\":0,\"startFlying\":%s}", latest[0], latest[1], latest[2], latest[1].let { kotlin.math.abs(it - 1.7f) > .01f }.toString())
                (getSystemService(CLIPBOARD_SERVICE) as ClipboardManager).setPrimaryClip(ClipData.newPlainText("spawn.json", json))
                Toast.makeText(this@MainActivity, "Координаты для spawn.json скопированы", Toast.LENGTH_SHORT).show()
            }
        }
        heading.addView(coordinates)
        notice = label("Свободный режим", 10f)
        heading.addView(notice)
        overlay.addView(heading, FrameLayout.LayoutParams(-2, -2, Gravity.TOP or Gravity.START))
        overlay.addView(label("+", 26f), FrameLayout.LayoutParams(-2, -2, Gravity.CENTER))
        // The reference HUD scales by available landscape height, not by a fixed phone resolution.
        val screenHeight = resources.configuration.screenHeightDp
        val actionSize = (screenHeight * .145f).toInt().coerceIn(60, 64)
        val stickSize = (screenHeight * .32f).toInt().coerceIn(120, 184)
        joystick = StickView(this) { right, forward ->
            // Behind the wheel the left stick is the gas pedal: up = forward, down = reverse.
            if(interactionCode==4){weapon.moving=false;nativeDriveThrottle(forward)}
            else {weapon.moving=kotlin.math.hypot(right,forward)>.1f;nativeMove(right, forward)}
        }
        overlay.addView(joystick, FrameLayout.LayoutParams(dp(stickSize), dp(stickSize), Gravity.BOTTOM or Gravity.START).apply {
            leftMargin = dp(14); bottomMargin = dp(22)
        })
        // Right stick only steers; the throttle lives on the left stick.
        driveStick = StickView(this) { steer, _ -> nativeDriveSteer(steer) }.apply { visibility = View.GONE }
        overlay.addView(driveStick, FrameLayout.LayoutParams(dp(stickSize), dp(stickSize), Gravity.BOTTOM or Gravity.END).apply {
            rightMargin = dp(14); bottomMargin = dp(22)
        })
        fun action(caption: String, icon: String, click: () -> Unit): HudButton = HudButton(this, caption, icon).apply {
            setOnClickListener { click() }
        }
        val pause = action("", "pause") { showPauseMenu() }.apply { visualScale=.73f }
        pause.contentDescription = "Пауза"
        overlay.addView(pause, FrameLayout.LayoutParams(dp(60), dp(60), Gravity.TOP or Gravity.END).apply { topMargin=dp(6); rightMargin=dp(8) })
        viewToggle=action("", "eye") { if(worldReady && active && !paused){ nativeCommand(46); viewToggle.isSelected=!viewToggle.isSelected } }.apply { visualScale=.73f; contentDescription="Сменить вид: от первого или от третьего лица" }
        overlay.addView(viewToggle, FrameLayout.LayoutParams(dp(60), dp(60), Gravity.TOP or Gravity.END).apply { topMargin=dp(126); rightMargin=dp(8) })
        val movement = LinearLayout(this).apply { orientation=LinearLayout.VERTICAL; gravity=Gravity.CENTER; isMotionEventSplittingEnabled=true }
        val jump = action("Прыжок", "jump") { nativeCommand(8) }
        // Fire on press, not release. The joystick keeps its own pointer and axes untouched.
        jump.setOnTouchListener { v, event ->
            when(event.actionMasked) {
                MotionEvent.ACTION_DOWN -> { v.isPressed=true;v.performClick() }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> v.isPressed=false
            }
            true
        }
        crouchControl = action("Присесть", "crouch") {
            crouchControl.isSelected = !crouchControl.isSelected; proneControl.isSelected=false
            nativeCommand(9)
        }
        proneControl = action("Лечь", "prone") {
            proneControl.isSelected = !proneControl.isSelected; crouchControl.isSelected=false
            nativeCommand(10)
        }
        for (control in listOf(jump,crouchControl)) movement.addView(control, LinearLayout.LayoutParams(dp(actionSize),dp(actionSize)))
        movementControls=movement
        overlay.addView(movement, FrameLayout.LayoutParams(-2,-2,Gravity.BOTTOM or Gravity.END).apply { bottomMargin=dp(8);rightMargin=dp(8) })
        toolWheel=ToolWheelView(this) { selectTool(it) }
        overlay.addView(toolWheel,FrameLayout.LayoutParams(-1,-1))
        // Click action is retained for accessibility; touch uses hold/drag/release.
        val switchWeapon=action("", "switch") {
            if(worldReady && active && !paused) AlertDialog.Builder(this)
                .setTitle("Выбор инструмента")
                .setItems(arrayOf("Грави-пушка", "Пульт FPV", "Пустые руки")) { _, index ->
                    selectTool(ToolWheelSelection.tools[index])
                }.show()
        }.apply { visualScale=.73f }
        switchWeapon.contentDescription="Выбор инструмента: удерживайте, проведите и отпустите"
        switchWeapon.setOnTouchListener { view, event ->
            if(!worldReady || !active || paused) { toolWheel.cancel(); true }
            else {
                if(event.actionMasked==MotionEvent.ACTION_DOWN) view.parent.requestDisallowInterceptTouchEvent(true)
                toolWheel.gesture(event,toolsState.tool) {
                    closePopup();lookArea.reset();toolWheel.bringToFront()
                }
            }
        }
        overlay.addView(switchWeapon,FrameLayout.LayoutParams(dp(60),dp(60),Gravity.TOP or Gravity.END).apply { topMargin=dp(66);rightMargin=dp(8) })
        catalogControl=action("", "cube") { showBuildCatalog() }.apply { contentDescription="Спавн: открыть каталог";visualScale=.73f }
        overlay.addView(catalogControl,FrameLayout.LayoutParams(dp(60),dp(60),Gravity.TOP or Gravity.END).apply { topMargin=dp(66);rightMargin=dp(72) })
        removeControl=action("", "remove") { nativeCommand(4) }.apply { danger=true;visualScale=.73f;contentDescription="Удалить" }
        overlay.addView(removeControl,FrameLayout.LayoutParams(dp(60),dp(60),Gravity.TOP or Gravity.END).apply { topMargin=dp(126);rightMargin=dp(72) })
        toolActions=LinearLayout(this).apply { orientation=LinearLayout.VERTICAL;gravity=Gravity.END;isMotionEventSplittingEnabled=true }
        spawnActions=LinearLayout(this).apply { orientation=LinearLayout.HORIZONTAL;gravity=Gravity.CENTER }
        gravityActions=LinearLayout(this).apply { orientation=LinearLayout.HORIZONTAL;gravity=Gravity.CENTER }
        fun addControl(parent: LinearLayout, caption: String, icon: String, click: () -> Unit): HudButton {
            val control=action(caption,icon,click)
            parent.addView(control,LinearLayout.LayoutParams(dp(72),dp(64)).apply { bottomMargin=dp(4) })
            return control
        }
        selectedSlot=FrameLayout(this).apply {
            background=GradientDrawable().apply { setColor(0xaa223b40.toInt());cornerRadius=dp(8).toFloat();setStroke(dp(1),0xff58dbb3.toInt()) }
            setOnClickListener { showBuildCatalog() }
        }
        selectedImage=ImageView(this).apply { scaleType=ImageView.ScaleType.FIT_CENTER }
        selectedSlot.addView(selectedImage,FrameLayout.LayoutParams(-1,-1))
        selectedSwatch=MaterialBlockView(this,selectedSurface)
        selectedSlot.addView(selectedSwatch,FrameLayout.LayoutParams(dp(25),dp(25),Gravity.BOTTOM or Gravity.END))
        // Show the current selection right next to the place button so it is always visible.
        spawnActions.addView(selectedSlot,LinearLayout.LayoutParams(dp(64),dp(64)).apply { bottomMargin=dp(4);rightMargin=dp(4) })
        addControl(spawnActions,"","build") { if(requestedTool==null)nativeCommand(200+BuildCatalog.entries[selected].nativeType) }.apply { contentDescription="Установить выбранный предмет" }
        val turns=LinearLayout(this).apply { orientation=LinearLayout.HORIZONTAL }
        addControl(turns,"","left") { nativeCommand(31) }.apply { contentDescription="Повернуть влево" }
        addControl(turns,"","right") { nativeCommand(2) }.apply { contentDescription="Повернуть вправо" }
        spawnActions.addView(turns)
        grabControl=addControl(gravityActions,"","grab") { nativeCommand(32) }.apply { contentDescription="Захват" }
        freezeControl=addControl(gravityActions,"Заморозить","freeze") { nativeCommand(38) }
        rotateHeldControl=addControl(gravityActions,"Вращать","rotate") { nativeCommand(33) }
        toolActions.addView(spawnActions)
        toolActions.addView(gravityActions)
        remoteActions=LinearLayout(this).apply { orientation=LinearLayout.HORIZONTAL;visibility=View.GONE }
        addControl(remoteActions,"Вернуть","home") { nativeCommand(44) }.apply { contentDescription="Вернуть дрон к игроку" }
        addControl(remoteActions,"Убрать пульт","remove") { nativeCommand(42) }.apply { contentDescription="Убрать пульт: пустые руки" }
        toolActions.addView(remoteActions)
        overlay.addView(toolActions,FrameLayout.LayoutParams(-2,-2,Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL).apply { bottomMargin=dp(8) })
        interactionControl=action("Открыть","grab") { nativeCommand(if(interactionCode==6)43 else 36) }.apply { visibility=View.GONE }
        overlay.addView(interactionControl,FrameLayout.LayoutParams(dp(80),dp(60),Gravity.BOTTOM or Gravity.END).apply { bottomMargin=dp(8);rightMargin=dp(76) })
        renderTools()
        // Flight is now in the pause menu; its altitude controls do not cover the action column.
        flight = Button(this)
        altitude = LinearLayout(this).apply { orientation=LinearLayout.VERTICAL;visibility=View.GONE }
        fun verticalButton(text: String, direction: Float) {
            altitude.addView(Button(this).apply {
                this.text=text;isAllCaps=false;textSize=11f
                setOnTouchListener { v,e ->
                    when(e.actionMasked) {
                        MotionEvent.ACTION_DOWN -> { nativeVertical(direction);v.isPressed=true }
                        MotionEvent.ACTION_UP,MotionEvent.ACTION_CANCEL -> { nativeVertical(0f);v.isPressed=false }
                    };true
                }
            },LinearLayout.LayoutParams(dp(84),dp(48)))
        }
        verticalButton("↑ Вверх",1f);verticalButton("↓ Вниз",-1f)
        overlay.addView(altitude,FrameLayout.LayoutParams(-2,-2,Gravity.CENTER_VERTICAL or Gravity.END).apply { rightMargin=dp(actionSize+20) })

        // Independent Android widgets remain responsive while GLB parsing runs on the native thread.
        loading = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL; gravity = Gravity.CENTER
            setBackgroundColor(Color.rgb(24, 35, 42)); isClickable = true; isFocusable = true
        }
        loading.addView(label("ЭКРОН", 32f))
        loading.addView(label("Пока ждёте загрузку мира, вам будет показан короткий видео ролик", 14f).apply { setPadding(0, dp(12), 0, dp(20)) })
        spinner = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply { max=100;isIndeterminate=false }
        loading.addView(spinner, LinearLayout.LayoutParams(dp(240), dp(16)))
        loadingText = label("Подготовка мира…", 14f).apply { gravity = Gravity.CENTER; setPadding(dp(20), dp(18), dp(20), 0) }
        loading.addView(loadingText)
        overlay.addView(loading, FrameLayout.LayoutParams(-1, -1))
        nativeReady(false)
        video = TextureView(this).also { texture ->
            texture.isOpaque = true; texture.isClickable = true
            overlay.addView(texture, FrameLayout.LayoutParams(-1, -1))
            texture.surfaceTextureListener = object : TextureView.SurfaceTextureListener {
                override fun onSurfaceTextureAvailable(st: SurfaceTexture, w: Int, h: Int) {
                    if (videoEndedAt != Long.MAX_VALUE) return
                    val surface = Surface(st)
                    try {
                        player = MediaPlayer().apply {
                            assets.openFd("intro.mp4").use { setDataSource(it.fileDescriptor, it.startOffset, it.length) }
                            setSurface(surface)
                            setOnPreparedListener { if (active) it.start() }
                            setOnCompletionListener { finishVideo() }
                            setOnErrorListener { _, _, _ -> finishVideo(); true }
                            prepareAsync()
                        }
                    } catch (_: Exception) { finishVideo() }
                    finally { surface.release() }
                }
                override fun onSurfaceTextureSizeChanged(st: SurfaceTexture, w: Int, h: Int) {}
                override fun onSurfaceTextureUpdated(st: SurfaceTexture) {}
                override fun onSurfaceTextureDestroyed(st: SurfaceTexture): Boolean {
                    if (videoEndedAt == Long.MAX_VALUE) finishVideo()
                    return true
                }
            }
        }
        dronePlace=action("Поставить дрон","build") { nativeCommand(41) }.apply { visibility=View.GONE }
        overlay.addView(dronePlace,FrameLayout.LayoutParams(dp(90),dp(70),Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL).apply { bottomMargin=dp(10) })
        fpvLayer=FrameLayout(this).apply { visibility=View.GONE;isMotionEventSplittingEnabled=true }
        fpvControls=FpvControlView(this,{throttle,yaw,pitch,roll->nativeDroneInput(throttle,yaw,pitch,roll)},{x,y->nativeLook(x,y)})
        fpvLayer.addView(fpvControls,FrameLayout.LayoutParams(-1,-1))
        fpvLayer.addView(action("Смена вида","switch") { nativeCommand(45) }.apply { contentDescription="Сменить вид: FPV или орбитальная камера" },FrameLayout.LayoutParams(dp(82),dp(64),Gravity.TOP or Gravity.END).apply { topMargin=dp(8);rightMargin=dp(76) })
        fpvLayer.addView(action("","remove") { fpvControls.reset();nativeCommand(42) }.apply { contentDescription="Выйти из FPV" },FrameLayout.LayoutParams(dp(60),dp(60),Gravity.TOP or Gravity.END).apply { topMargin=dp(8);rightMargin=dp(8) })
        overlay.addView(fpvLayer,FrameLayout.LayoutParams(-1,-1))
        addContentView(overlay, ViewGroup.LayoutParams(-1, -1))
        hideSystemUi()
        catalogExecutor.execute {
            for(index in BuildCatalog.entries.indices) {
                val bitmap=try { assets.open(BuildCatalog.entries[index].asset).use { BitmapFactory.decodeStream(it) } } catch (_: Exception) { null }
                handler.post { if(!isDestroyed){catalogImages[index]=bitmap;catalogAdapter?.notifyDataSetChanged();updateSelectedIcon()} }
            }
            val paths=mapOf(5 to "wood",1 to "concrete",2 to "brick",3 to "rust",6 to "fabric")
            for((id,name) in paths){
                val bitmap=try{assets.open("textures/$name.png").use { BitmapFactory.decodeStream(it) }}catch(_:Exception){null}
                handler.post { if(!isDestroyed){materialImages[id]=bitmap;catalogAdapter?.notifyDataSetChanged();updateSelectedIcon()} }
            }
        }
    }
    private fun updateSelectedIcon(){
        if(!::selectedImage.isInitialized)return
        val image=catalogImages[selected]
        if(image!=null)selectedImage.setImageBitmap(image) else selectedImage.setImageResource(android.R.drawable.ic_menu_gallery)
        selectedSwatch.materialId=selectedSurface;selectedSwatch.texture=materialImages.getOrNull(selectedSurface);selectedSwatch.invalidate()
        val material=if(selectedSurface<0) "Материалы модели" else if(selectedSurface==6) "Ткань" else materialNames[materialIds.indexOf(selectedSurface).coerceAtLeast(0)]
        selectedSlot.contentDescription="${items[selected]} · $material · открыть каталог"
    }
    private fun closePopup(){popup?.let { hud.removeView(it) };popup=null;catalogAdapter=null}
    private fun panel(title: String): LinearLayout {
        closePopup()
        val panel=LinearLayout(this).apply {
            orientation=LinearLayout.VERTICAL;isClickable=true;isMotionEventSplittingEnabled=true
            background=GradientDrawable().apply { setColor(0xe6202b33.toInt());cornerRadius=dp(12).toFloat();setStroke(dp(1),0x887eaaa6.toInt()) }
            setPadding(dp(8),dp(4),dp(8),dp(6))
        }
        val heading=LinearLayout(this).apply { orientation=LinearLayout.HORIZONTAL;gravity=Gravity.CENTER_VERTICAL }
        heading.addView(label(title,13f),LinearLayout.LayoutParams(0,dp(32),1f))
        heading.addView(label("×",24f).apply { gravity=Gravity.CENTER;contentDescription="Закрыть каталог";setOnClickListener { closePopup() } },LinearLayout.LayoutParams(dp(48),dp(32)))
        panel.addView(heading)
        val height=resources.configuration.screenHeightDp
        val left=(height*.32f).toInt().coerceIn(120,184)+28
        // Large grid area: keep only the right-side controls clear, use most of the screen height.
        hud.addView(panel,FrameLayout.LayoutParams((hud.width-dp(left+96)).coerceAtLeast(dp(260)),dp((height-104).coerceAtLeast(152)),Gravity.TOP or Gravity.START).apply { leftMargin=dp(left);topMargin=dp(52) })
        popup=panel;return panel
    }
    private fun renderTools() {
        weapon.tool=toolsState.tool
        weapon.visibility=if(toolsState.viewmodelVisible&&interactionCode!=4)View.VISIBLE else View.GONE
        // Jump/crouch sit on top of the drive stick corner; hide them while driving.
        if(::movementControls.isInitialized)movementControls.visibility=if(interactionCode!=4)View.VISIBLE else View.GONE
        updateSelectedIcon()
        catalogControl.visibility=if(interactionCode!=4)View.VISIBLE else View.GONE
        spawnActions.visibility=if(toolsState.spawnVisible&&interactionCode!=4)View.VISIBLE else View.GONE
        remoteActions.visibility=if(toolsState.tool==HandTool.DRONE_REMOTE&&interactionCode!=4)View.VISIBLE else View.GONE
        gravityActions.visibility=if(toolsState.gravityVisible&&interactionCode!=4)View.VISIBLE else View.GONE
        removeControl.visibility=if(toolsState.deleteVisible&&interactionCode!=4)View.VISIBLE else View.GONE
        rotateHeldControl.visibility=if(toolsState.rotateHeldVisible&&interactionCode!=4)View.VISIBLE else View.GONE
        if(grabControl.contentDescription!=toolsState.grabCaption) {
            grabControl.symbol=if(toolsState.holding) "release" else "grab"
            grabControl.contentDescription=toolsState.grabCaption;grabControl.invalidate()
        }
    }
    private fun selectTool(tool: HandTool) {
        closePopup();requestedTool=tool;toolsState.select(tool);nativeCommand(20+tool.ordinal);renderTools()
    }
    private fun showBuildCatalog(category: Int = 0) {
        if(!worldReady || !active || paused || interactionCode==4)return
        val container=panel("Спавн")
        val tabs=LinearLayout(this).apply { orientation=LinearLayout.HORIZONTAL }
        listOf("Строительство", "Мебель", "Оружие", "FPV Дроны").forEachIndexed { index, title ->
            tabs.addView(Button(this).apply { text=title;textSize=10f;isSelected=index==category
                setTextColor(if(index==category)0xff7dffd2.toInt() else Color.WHITE)
                setOnClickListener { showBuildCatalog(index) }
            },LinearLayout.LayoutParams(0,dp(48),1f))
        }
        container.addView(tabs)
        val visibleItems=BuildCatalog.indicesForCategory(category)
        if(category==3){
            val cards=LinearLayout(this).apply { orientation=LinearLayout.HORIZONTAL }
            fun card(title:String,asset:String,choose:()->Unit){
                val image=ImageView(this).apply { scaleType=ImageView.ScaleType.FIT_CENTER;contentDescription=title }
                val item=LinearLayout(this).apply { orientation=LinearLayout.VERTICAL;gravity=Gravity.CENTER;isClickable=true;contentDescription=title;setOnClickListener { choose() } }
                item.addView(image,LinearLayout.LayoutParams(-1,0,1f));item.addView(label(title,12f).apply { gravity=Gravity.CENTER },LinearLayout.LayoutParams(-1,dp(36)))
                cards.addView(item,LinearLayout.LayoutParams(0,-1,1f))
                catalogExecutor.execute {
                    val bitmap=try { assets.open(asset).use { BitmapFactory.decodeStream(it) } }catch(_:Exception){null}
                    handler.post { if(!isDestroyed)image.setImageBitmap(bitmap) }
                }
            }
            card("Пульт FPV · взять", "catalog/rendered/fpv_remote.webp"){selectTool(HandTool.DRONE_REMOTE)}
            card("FPV-дрон · достать", "catalog/rendered/fpv_drone.webp"){nativeCommand(40);closePopup()}
            container.addView(cards,LinearLayout.LayoutParams(-1,0,1f))
            return
        }
        if(visibleItems.isEmpty()){
            container.addView(label("В этом разделе пока нет предметов",14f).apply { gravity=Gravity.CENTER },LinearLayout.LayoutParams(-1,0,1f))
            return
        }
        val grid=GridView(this).apply { numColumns=4;horizontalSpacing=dp(4);verticalSpacing=dp(4);clipToPadding=false;isMotionEventSplittingEnabled=true }
        val adapter=object : BaseAdapter() {
            override fun getCount()=visibleItems.size
            override fun getItem(position: Int)=items[visibleItems[position]]
            override fun getItemId(position: Int)=visibleItems[position].toLong()
            override fun getView(row: Int, convertView: View?, parent: ViewGroup): View = LinearLayout(this@MainActivity).apply {
                val position=visibleItems[row]
                orientation=LinearLayout.VERTICAL;gravity=Gravity.CENTER
                contentDescription=items[position];minimumHeight=dp(88)
                layoutParams=AbsListView.LayoutParams(-1,dp(88))
                background=GradientDrawable().apply {setColor(if(position==selected)0xaa477264.toInt() else 0x443b4652);cornerRadius=dp(6).toFloat();if(position==selected)setStroke(dp(1),0xff92d9ac.toInt())}
                val illustration: View = ImageView(this@MainActivity).apply {
                    scaleType=ImageView.ScaleType.FIT_CENTER
                    val image=catalogImages[position]
                    if(image!=null)setImageBitmap(image) else setImageResource(android.R.drawable.ic_menu_gallery)
                }
                illustration.isClickable=false;illustration.isFocusable=false
                addView(illustration,LinearLayout.LayoutParams(-1,0,1f))
                addView(label(items[position],10f).apply { gravity=Gravity.CENTER;isClickable=false;maxLines=2 },LinearLayout.LayoutParams(-1,dp(28)))
            }
        }
        catalogAdapter=adapter;grid.adapter=adapter;container.addView(grid,LinearLayout.LayoutParams(-1,0,1f))
        grid.setOnItemClickListener { _,_,row,_ ->
            val index=visibleItems[row]
            requestedTool=HandTool.SPAWN_GUN;toolsState.select(HandTool.SPAWN_GUN)
            nativeCommand(300+BuildCatalog.entries[index].nativeType)
            pendingSelection=index;selected=index;selectedSurface=BuildCatalog.entries[index].defaultSurface
            updateSelectedIcon();closePopup();renderTools()
        }
    }
    private fun setPaused(value: Boolean) {
        if(::toolWheel.isInitialized)toolWheel.cancel()
        closePopup();paused=value;joystick.reset();lookArea.reset();if(::driveStick.isInitialized)driveStick.reset();weapon.running=!value && active;nativeVertical(0f)
        nativeReady(worldReady && active && !paused)
    }
    private fun showPauseMenu() {
        setPaused(true)
        val dialog=AlertDialog.Builder(this).setTitle("Пауза")
            .setItems(arrayOf("Продолжить",if(flyingNow) "Выключить полёт" else "Включить полёт","На дорогу","Восстановить УАЗ","На спавн","Камера: 1-е / 3-е лицо","Отчёт о вылете")) { _, which ->
                setPaused(false)
                if(which==1)nativeCommand(6)
                if(which==2)nativeCommand(7)
                if(which==3)nativeCommand(34)
                if(which==4)nativeCommand(35)
                if(which==5)nativeCommand(46)
                if(which==6)handler.post { showExitReport() }
            }.create()
        dialog.setOnDismissListener { setPaused(false);hideSystemUi() }
        dialog.show()
    }
    @Volatile private var exitReport = "Отчёт ещё готовится. Откройте этот пункт через несколько секунд."
    private fun collectExitReport() {
        catalogExecutor.execute {
            val report = runCatching {
                val manager=getSystemService(ACTIVITY_SERVICE) as android.app.ActivityManager
                val memory=android.app.ActivityManager.MemoryInfo().also { manager.getMemoryInfo(it) }
                val exits=manager.getHistoricalProcessExitReasons(packageName,0,3)
                buildString {
                    append("ЭКРОН v0.52\nУстройство: ${android.os.Build.MANUFACTURER} ${android.os.Build.MODEL}\nAndroid: ${android.os.Build.VERSION.RELEASE}\n")
                    append("RAM: ${memory.totalMem/1048576} MB, свободно ${memory.availMem/1048576} MB\n")
                    if(exits.isEmpty())append("Android не сохранил сведения о предыдущем завершении.\n")
                    for(e in exits){
                        val reason=when(e.reason){
                            android.app.ApplicationExitInfo.REASON_CRASH_NATIVE -> "Нативный сбой"
                            android.app.ApplicationExitInfo.REASON_CRASH -> "Java/Kotlin сбой"
                            android.app.ApplicationExitInfo.REASON_LOW_MEMORY -> "Недостаточно памяти"
                            android.app.ApplicationExitInfo.REASON_ANR -> "Приложение не отвечает"
                            else -> "Код ${e.reason}"
                        }
                        append("\n${java.util.Date(e.timestamp)}: $reason, status=${e.status}, PSS=${e.pss} KB, RSS=${e.rss} KB\n${e.description.orEmpty()}\n")
                        runCatching { if(e.reason==android.app.ApplicationExitInfo.REASON_ANR) e.traceInputStream?.use { input ->
                            val data=ByteArray(8000);val n=input.read(data);if(n>0)append(String(data,0,n,Charsets.UTF_8))
                            Unit
                        }; Unit }
                    }
                }.take(28000)
            }.getOrElse { "Не удалось получить историю завершений: ${it.javaClass.simpleName}. Пришлите модель телефона и действия перед вылетом." }
            exitReport=report
        }
    }
    private fun showExitReport(){
        setPaused(true)
        val report=exitReport
        AlertDialog.Builder(this).setTitle("Отчёт о завершении игры").setMessage(report)
            .setPositiveButton("Скопировать") { _,_ ->
                (getSystemService(CLIPBOARD_SERVICE) as ClipboardManager).setPrimaryClip(ClipData.newPlainText("Ekron crash",report))
            }.setNegativeButton("Закрыть",null).setOnDismissListener { setPaused(false);hideSystemUi() }.show()
    }
    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        if(keyCode==KeyEvent.KEYCODE_V && worldReady && !paused){nativeCommand(46);return true}
        return super.onKeyUp(keyCode,event)
    }
    override fun onResume() {
        super.onResume()
        active = true
        try { player?.start() } catch (_: IllegalStateException) {}
        if (::coordinates.isInitialized) { handler.removeCallbacks(poll); handler.post(poll) }
    }
    override fun onPause() {
        if(::toolWheel.isInitialized)toolWheel.cancel()
        if(::fpvControls.isInitialized)fpvControls.reset()
        closePopup();weapon.running=false
        active = false; nativeReady(false)
        if (::joystick.isInitialized) joystick.reset();if (::driveStick.isInitialized) driveStick.reset();if (::lookArea.isInitialized)lookArea.reset()
        try { player?.pause() } catch (_: IllegalStateException) {}
        handler.removeCallbacks(poll); nativeVertical(0f)
        super.onPause()
    }
    private fun finishVideo() {
        if (videoEndedAt != Long.MAX_VALUE) return
        videoEndedAt = SystemClock.elapsedRealtime()
        player?.release(); player = null; video?.visibility = View.GONE
    }
    override fun onDestroy() { catalogExecutor.shutdownNow();nativeReady(false); player?.release(); player = null; handler.removeCallbacks(poll); super.onDestroy() }
    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) hideSystemUi() else { nativeVertical(0f); if (::joystick.isInitialized) joystick.reset();if (::driveStick.isInitialized) driveStick.reset();if (::lookArea.isInitialized)lookArea.reset() }
    }
    @Suppress("DEPRECATION")
    private fun hideSystemUi() {
        window.decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            or View.SYSTEM_UI_FLAG_LAYOUT_STABLE or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            or View.SYSTEM_UI_FLAG_FULLSCREEN)
    }
}
