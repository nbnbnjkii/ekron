#pragma once
#include "LobbyScene.h"
#include <GLES3/gl3.h>
#include <cstddef>
namespace sandbox {
class LobbyRenderer {
 GLuint program_=0,buffer_=0,vao_=0;std::array<GLuint,4> textures_{};
 static GLuint shader(GLenum type,const char*source){GLuint s=glCreateShader(type);glShaderSource(s,1,&source,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;}
public:
 bool initialize(){
  const char*vs=R"(#version 300 es
layout(location=0) in vec3 position;layout(location=1) in vec3 color;layout(location=2) in vec3 normal;layout(location=3) in vec2 uv;
uniform vec3 eye;uniform vec2 angles;uniform float aspect;out vec3 tint;out vec2 texUV;
void main(){vec3 p=position-eye;float c=cos(angles.x),s=sin(angles.x),cp=cos(angles.y),sp=sin(angles.y);vec3 q=vec3(c*p.x-s*p.z,p.y,s*p.x+c*p.z);q=vec3(q.x,cp*q.y+sp*q.z,-sp*q.y+cp*q.z);gl_Position=vec4(q.x*1.37638/aspect,q.y*1.37638,-1.000100*q.z-.200010,-q.z);tint=color*(.6+.4*abs(normal.y));texUV=uv;})";
  const char*fs=R"(#version 300 es
precision mediump float;in vec3 tint;in vec2 texUV;uniform bool screen;uniform sampler2D image;out vec4 frag;
void main(){frag=screen?vec4(texture(image,texUV).rgb,1):vec4(tint,1);})";
  GLuint v=shader(GL_VERTEX_SHADER,vs),f=shader(GL_FRAGMENT_SHADER,fs);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return false;}
  program_=glCreateProgram();glAttachShader(program_,v);glAttachShader(program_,f);glLinkProgram(program_);glDeleteShader(v);glDeleteShader(f);
  GLint ok=0;glGetProgramiv(program_,GL_LINK_STATUS,&ok);if(!ok){release();return false;}
  glGenVertexArrays(1,&vao_);glGenBuffers(1,&buffer_);glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,buffer_);
  for(int i=0;i<4;i++){glEnableVertexAttribArray(i);glVertexAttribPointer(i,i==3?2:3,GL_FLOAT,GL_FALSE,sizeof(MeshVertex),reinterpret_cast<void*>((i==0?0:i==1?3:i==2?6:9)*sizeof(float)));}
  glGenTextures(4,textures_.data());for(GLuint t:textures_){glBindTexture(GL_TEXTURE_2D,t);unsigned char pixel[4]={18,35,26,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixel);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}return true;
 }
 void image(int index,const unsigned char*bytes){glBindTexture(GL_TEXTURE_2D,textures_[index]);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,768,432,0,GL_RGBA,GL_UNSIGNED_BYTE,bytes);}
 void draw(const LobbyScene&scene,int width,int height){
  glViewport(0,0,width,height);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glClearColor(.05f,.08f,.06f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glUseProgram(program_);glUniform3f(glGetUniformLocation(program_,"eye"),scene.x,scene.y,scene.z);glUniform2f(glGetUniformLocation(program_,"angles"),scene.yaw,scene.pitch);glUniform1f(glGetUniformLocation(program_,"aspect"),float(width)/height);
  glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,buffer_);glUniform1i(glGetUniformLocation(program_,"screen"),0);
  glBufferData(GL_ARRAY_BUFFER,scene.room.size()*sizeof(MeshVertex),scene.room.data(),GL_STATIC_DRAW);glDrawArrays(GL_TRIANGLES,0,GLsizei(scene.room.size()));
  glActiveTexture(GL_TEXTURE0);glUniform1i(glGetUniformLocation(program_,"image"),0);glUniform1i(glGetUniformLocation(program_,"screen"),1);
  for(int i=0;i<4;i++){std::array<MeshVertex,6> vertices{};int j=0;for(int corner:{0,1,2,0,2,3}){float u=corner==1||corner==2?1:0,v=corner>=2?1:0;auto p=LobbyScene::screenPoint(i,u,v);vertices[j++]={p.x,p.y,p.z,1,1,1,0,1,0,u,v,0};}
   glBindTexture(GL_TEXTURE_2D,textures_[i]);glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,6);
  }
 }
 void release(){if(buffer_)glDeleteBuffers(1,&buffer_);if(vao_)glDeleteVertexArrays(1,&vao_);if(program_)glDeleteProgram(program_);glDeleteTextures(4,textures_.data());program_=buffer_=vao_=0;textures_.fill(0);}
};
}
