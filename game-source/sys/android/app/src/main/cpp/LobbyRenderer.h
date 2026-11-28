#pragma once
#include "LobbyScene.h"
#include "LobbyArtwork.h"
#include <GLES3/gl3.h>
#include <cstddef>
namespace sandbox {
class LobbyRenderer {
 GLuint program_=0,buffer_=0,vao_=0;std::array<GLuint,4> textures_{};
 GLuint artwork_=0;bool artworkReady_=false;std::future<std::vector<unsigned char>> artworkJob_;
 GLuint roomBuffer_=0,roomVao_=0;GLsizei roomCount_=0;
 static GLuint shader(GLenum type,const char*source){GLuint s=glCreateShader(type);glShaderSource(s,1,&source,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;}
public:
 bool initialize(AAssetManager*assets){
  const char*vs=R"(#version 300 es
layout(location=0) in vec3 position;layout(location=1) in vec3 color;layout(location=2) in vec3 normal;layout(location=3) in vec2 uv;layout(location=4) in float material;
uniform vec3 eye;uniform vec2 angles;uniform float aspect;out vec3 tint;out vec2 texUV;out vec3 worldPos;out vec3 worldNormal;flat out highp int surface;
void main(){vec3 p=position-eye;float c=cos(angles.x),s=sin(angles.x),cp=cos(angles.y),sp=sin(angles.y);vec3 q=vec3(c*p.x-s*p.z,p.y,s*p.x+c*p.z);q=vec3(q.x,cp*q.y+sp*q.z,-sp*q.y+cp*q.z);gl_Position=vec4(q.x*1.37638/aspect,q.y*1.37638,-1.000100*q.z-.200010,-q.z);tint=color;texUV=uv;worldPos=position;worldNormal=normal;surface=int(material+.5);})";
  const char*fs=R"(#version 300 es
precision highp float;
in vec3 tint;in vec2 texUV;in vec3 worldPos;in vec3 worldNormal;flat in highp int surface;
uniform bool screen;uniform sampler2D image;uniform sampler2D artwork;uniform bool artworkReady;uniform vec3 eye;uniform float time;out vec4 frag;
float hash(vec3 p){return fract(sin(dot(p,vec3(12.9898,78.233,37.719)))*43758.5453);}
void main(){
 if(screen){frag=vec4(texture(image,texUV).rgb,1);return;}
 vec3 p=worldPos,n=normalize(worldNormal),base=tint;
 if(surface>=16&&artworkReady){int tile=surface-16;vec2 tileUV=vec2(float(tile%4),float(tile/4));vec4 texel=texture(artwork,(tileUV*512.0+vec2(2.5)+clamp(texUV,0.0,1.0)*507.0)/2048.0);if(texel.a<.4)discard;base=texel.rgb;}
 vec2 wallUV=abs(n.z)>.5?p.xy:p.zy;
 float grain=hash(floor(p*140.0));
 if(surface==0){base*=.84+.18*grain;base*=mix(.76,1.0,smoothstep(0.0,.09,abs(fract(wallUV.y*.7)-.5)));}
 if(surface==3){vec2 grid=abs(fract(p.xz*.5)-.5);base*=.82+.17*grain;base*=mix(.6,1.0,smoothstep(.0,.018,min(grid.x,grid.y)));}
 if(surface==4){float star=cos(atan(texUV.y-.5,texUV.x-.5)*5.0);if(length((texUV-.5)*vec2(1,2.5))<.21+.07*star)base=vec3(.78,.63,.27);}
 if(surface==5){vec2 grid=abs(fract(texUV*vec2(9,6))-.5);base*=mix(.48,1.0,smoothstep(0.0,.025,min(grid.x,grid.y)));float route=abs(texUV.y-(.2+.55*texUV.x+.06*sin(texUV.x*20.0)));if(route<.012)base=vec3(.48,.14,.08);}
 if(surface==6){base*=.85+.20*grain;float slat=step(.94,fract(texUV.y*6.0));base*=1.0-.4*slat;}
 if(surface==7)base*=.83+.22*grain;
 if(surface==8||surface==9){float stripe=step(.5,fract((surface==9?wallUV.x+wallUV.y:texUV.x+texUV.y)*8.0));if(surface==9||abs(texUV.y-.5)<.17)base=mix(vec3(.06),vec3(.65,.49,.13),stripe);}
 if(surface==10){if(texUV.y<.25&&fract(texUV.y*35.0)>.6)base*=.3;}
 if(surface==11){frag=vec4(base,.22);return;}
 if(surface==2){frag=vec4(base*(.91+.09*sin(time*2.0)),1);return;}
 vec3 illumination=vec3(.30,.35,.30);
 for(int i=0;i<4;i++){vec3 lp=vec3((i%2==0?-3.6:3.6),4.5,(i<2?-3.6:3.6));vec3 d=lp-p;float falloff=1.8/(1.0+dot(d,d)*.12);illumination+=vec3(1,.9,.72)*max(0.0,dot(n,normalize(d)))*falloff;}
 float wallDistance=8.0-max(abs(p.x),abs(p.z));illumination+=vec3(.05,.22,.11)*exp(-max(0.0,wallDistance)*.7);
 vec3 lit=base*illumination;float fog=1.0-exp(-length(p-eye)*.012);frag=vec4(mix(lit,vec3(.025,.04,.03),fog),1);
})";
  GLuint v=shader(GL_VERTEX_SHADER,vs),f=shader(GL_FRAGMENT_SHADER,fs);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return false;}
  program_=glCreateProgram();glAttachShader(program_,v);glAttachShader(program_,f);glLinkProgram(program_);glDeleteShader(v);glDeleteShader(f);
  GLint ok=0;glGetProgramiv(program_,GL_LINK_STATUS,&ok);if(!ok){release();return false;}
  glGenVertexArrays(1,&vao_);glGenBuffers(1,&buffer_);glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,buffer_);
  for(int i=0;i<5;i++){glEnableVertexAttribArray(i);glVertexAttribPointer(i,i==4?1:i==3?2:3,GL_FLOAT,GL_FALSE,sizeof(MeshVertex),reinterpret_cast<void*>((i==0?0:i==1?3:i==2?6:i==3?9:11)*sizeof(float)));}
  glGenBuffers(1,&roomBuffer_);glGenVertexArrays(1,&roomVao_);
  glGenTextures(1,&artwork_);glBindTexture(GL_TEXTURE_2D,artwork_);unsigned char blank[4]={40,60,45,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,blank);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  artworkJob_=std::async(std::launch::async,[assets](){return readLobbyArtwork(assets);});
  glGenTextures(4,textures_.data());for(GLuint t:textures_){glBindTexture(GL_TEXTURE_2D,t);unsigned char pixel[4]={18,35,26,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixel);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}return true;
 }
 void image(int index,const unsigned char*bytes){glBindTexture(GL_TEXTURE_2D,textures_[index]);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,768,432,0,GL_RGBA,GL_UNSIGNED_BYTE,bytes);}
 void draw(const LobbyScene&scene,int width,int height){
  glViewport(0,0,width,height);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glClearColor(.05f,.08f,.06f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,artwork_);
  if(artworkJob_.valid()&&artworkJob_.wait_for(std::chrono::seconds(0))==std::future_status::ready){
   try{auto pixels=artworkJob_.get();if(!pixels.empty()){glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2048,2048,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());artworkReady_=true;}}catch(...){artworkReady_=false;}
  }
  glUseProgram(program_);glUniform1i(glGetUniformLocation(program_,"artwork"),1);glUniform1i(glGetUniformLocation(program_,"artworkReady"),artworkReady_?1:0);
  glUniform3f(glGetUniformLocation(program_,"eye"),scene.x,scene.y,scene.z);glUniform2f(glGetUniformLocation(program_,"angles"),scene.yaw,scene.pitch);glUniform1f(glGetUniformLocation(program_,"aspect"),float(width)/height);
  glUniform1f(glGetUniformLocation(program_,"time"),scene.clock);
  if(!roomCount_){glBindVertexArray(roomVao_);glBindBuffer(GL_ARRAY_BUFFER,roomBuffer_);for(int i=0;i<5;i++){glEnableVertexAttribArray(i);glVertexAttribPointer(i,i==4?1:i==3?2:3,GL_FLOAT,GL_FALSE,sizeof(MeshVertex),reinterpret_cast<void*>((i==0?0:i==1?3:i==2?6:i==3?9:11)*sizeof(float)));}glBufferData(GL_ARRAY_BUFFER,scene.room.size()*sizeof(MeshVertex),scene.room.data(),GL_STATIC_DRAW);roomCount_=GLsizei(scene.room.size());}
  glBindVertexArray(roomVao_);glUniform1i(glGetUniformLocation(program_,"screen"),0);
  glDrawArrays(GL_TRIANGLES,0,roomCount_);
  glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,buffer_);
  if(!scene.effects.empty()){glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);glBufferData(GL_ARRAY_BUFFER,scene.effects.size()*sizeof(MeshVertex),scene.effects.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,GLsizei(scene.effects.size()));glDepthMask(GL_TRUE);glDisable(GL_BLEND);}
  glActiveTexture(GL_TEXTURE0);glUniform1i(glGetUniformLocation(program_,"image"),0);glUniform1i(glGetUniformLocation(program_,"screen"),1);
  for(int i=0;i<4;i++){std::array<MeshVertex,6> vertices{};int j=0;for(int corner:{0,1,2,0,2,3}){float u=corner==1||corner==2?1:0,v=corner>=2?1:0;auto p=LobbyScene::screenPoint(i,u,v);vertices[j++]={p.x,p.y,p.z,1,1,1,0,1,0,u,v,0};}
   glBindTexture(GL_TEXTURE_2D,textures_[i]);glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,6);
  }
 }
 void release(){if(artworkJob_.valid())artworkJob_.wait();if(artwork_)glDeleteTextures(1,&artwork_);artwork_=0;artworkReady_=false;if(roomBuffer_)glDeleteBuffers(1,&roomBuffer_);if(roomVao_)glDeleteVertexArrays(1,&roomVao_);roomBuffer_=roomVao_=0;roomCount_=0;if(buffer_)glDeleteBuffers(1,&buffer_);if(vao_)glDeleteVertexArrays(1,&vao_);if(program_)glDeleteProgram(program_);glDeleteTextures(4,textures_.data());program_=buffer_=vao_=0;textures_.fill(0);}
};
}
