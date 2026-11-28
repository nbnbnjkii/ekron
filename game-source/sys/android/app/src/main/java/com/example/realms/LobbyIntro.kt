package com.example.realms

import android.content.Context
import android.graphics.SurfaceTexture
import android.media.MediaPlayer
import android.view.Surface
import android.view.TextureView

/** Continuous platform video decoding; no bitmap extraction, seeks or UI frame copies. */
class LobbyIntro(context:Context, private val failed:()->Unit):TextureView(context),TextureView.SurfaceTextureListener {
    private var player:MediaPlayer?=null
    private var surface:Surface?=null
    private var prepared=false
    private var closed=false
    private var muted=context.getSharedPreferences("settings",0).getBoolean("muted",false)
    var screenVisible=false
        set(value){if(field!=value){field=value;reconcile()}}
    var playing=true
        set(value){field=value;reconcile()}
    init {isOpaque=false;isClickable=false;surfaceTextureListener=this;alpha=0f}
    fun setMuted(value:Boolean){muted=value;player?.let { runCatching { it.setVolume(if(value)0f else 1f,if(value)0f else 1f) } }}
    private fun reconcile(){
        alpha=if(screenVisible&&prepared&&!closed)1f else 0f
        val p=player?:return
        if(prepared)runCatching { if(playing&&screenVisible&&windowVisibility==VISIBLE&&isShown)p.start() else p.pause() }
    }
    override fun onSurfaceTextureAvailable(texture:SurfaceTexture,w:Int,h:Int){
        if(closed)return
        surface=Surface(texture)
        if(player!=null){player?.setSurface(surface);reconcile();return}
        try {
            player=MediaPlayer().also { p->
                context.assets.openFd("intro.mp4").use { p.setDataSource(it.fileDescriptor,it.startOffset,it.length) }
                p.setSurface(surface);p.isLooping=true
                p.setOnPreparedListener { if(!closed){prepared=true;setMuted(muted);reconcile()} }
                p.setOnErrorListener { _,_,_->prepared=false;reconcile();failed();true }
                p.prepareAsync()
            }
        }catch(_:Exception){prepared=false;reconcile();failed()}
    }
    override fun onSurfaceTextureDestroyed(texture:SurfaceTexture):Boolean{
        runCatching { player?.pause();player?.setSurface(null) };surface?.release();surface=null;return true
    }
    override fun onSurfaceTextureSizeChanged(texture:SurfaceTexture,w:Int,h:Int){}
    override fun onSurfaceTextureUpdated(texture:SurfaceTexture){}
    override fun onVisibilityChanged(changedView:android.view.View,visibility:Int){super.onVisibilityChanged(changedView,visibility);reconcile()}
    override fun onWindowVisibilityChanged(visibility:Int){super.onWindowVisibilityChanged(visibility);reconcile()}
    fun close(){closed=true;prepared=false;runCatching { player?.release() };player=null;surface?.release();surface=null}
}
