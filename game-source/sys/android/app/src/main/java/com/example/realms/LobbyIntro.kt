package com.example.realms

import android.content.Context
import android.graphics.Bitmap
import android.media.MediaMetadataRetriever
import android.os.Handler
import android.os.Looper
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

/** One bounded decoder worker, one pending UI frame, never decode on the UI thread. */
class LobbyIntro(context:Context, private val frame:(Bitmap)->Unit, private val failed:()->Unit) {
    @Volatile var playing=true
    private val closed=AtomicBoolean(false)
    private val pending=AtomicBoolean(false)
    private val handler=Handler(Looper.getMainLooper())
    private val worker=Executors.newSingleThreadScheduledExecutor()
    private val retriever=MediaMetadataRetriever()
    private var ready=false
    private var position=0L
    private var duration=1L
    init {
        worker.execute {
            try {
                context.assets.openFd("intro.mp4").use { retriever.setDataSource(it.fileDescriptor,it.startOffset,it.length) }
                duration=(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION)?.toLongOrNull()?:1L).coerceAtLeast(1L)
                ready=true
            }catch(_:Exception){handler.post { if(!closed.get())failed() }}
        }
        worker.scheduleWithFixedDelay({
            if(!ready||!playing||closed.get()||!pending.compareAndSet(false,true))return@scheduleWithFixedDelay
            try {
                val image=retriever.getScaledFrameAtTime(position*1000,MediaMetadataRetriever.OPTION_CLOSEST,640,360)
                position=(position+125)%duration
                if(image==null){pending.set(false)}else handler.post {
                    try { if(!closed.get())frame(image) }finally{image.recycle();pending.set(false)}
                }
            }catch(_:Exception){pending.set(false)}
        },0,125,TimeUnit.MILLISECONDS)
    }
    fun close(){
        if(!closed.compareAndSet(false,true))return
        worker.execute { retriever.release() }
        worker.shutdown() // scheduled tasks are cancelled; running decode finishes before release
    }
}
