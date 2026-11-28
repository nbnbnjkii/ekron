#pragma once
#include "MeshGeometry.h"
#include "CollisionMesh.h"
#include <cmath>
namespace sandbox {
// Equipment is separate from the tool wheel and object IDs/save protocol.
struct Jetpack {
 enum State {Absent,InHands,Worn};State state=Absent;
 bool take(){if(state==Worn)return false;state=InHands;return true;}
 bool wear(){if(state!=InHands)return false;state=Worn;return true;}
 void remove(){state=Absent;air_=0;}
 bool thrustAllowed(bool seated,bool piloting,bool crouched,bool prone)const{return state==Worn&&!seated&&!piloting&&!crouched&&!prone;}
 // Air wisps are world geometry, not a screen overlay or a fire effect.
 float phase_=0,air_=0;
 void update(float dt,bool active,float effort){
  dt=std::isfinite(dt)?std::clamp(dt,0.f,.05f):0.f;
  phase_=std::fmod(phase_+dt*1.8f,1.f);
  float target=active&&state==Worn?.35f+.65f*std::clamp(effort,0.f,1.f):0.f;
  air_+=(target-air_)*(1-std::exp(-12.f*dt));
 }
 void appendExhaust(std::vector<MeshVertex>&out,physics::Vec3 eye,float yaw)const{
  if(state!=Worn||air_<.015f)return;
  constexpr int sides=12;
  const float c=std::cos(yaw),s=std::sin(yaw);
  auto vertex=[&](float x,float y,float z,float nx,float nz,float light){
   return MeshVertex{eye.x+c*x-s*z,eye.y+y,eye.z-s*x-c*z,
    light*.78f,light*.91f,light,c*nx-s*nz,0,-s*nx-c*nz,0,0,float(Plain)};
  };
  for(float nozzle:{-.16f,.16f})for(int ring=0;ring<7;ring++){
   float t=std::fmod(phase_+ring/7.f,1.f);
   float radius=.045f+t*.15f*air_,thickness=.006f*(1-t)*air_+.001f;
   float y=-.94f-t*(.45f+.65f*air_),z=-.41f;
   float light=.35f+.55f*(1-t)*air_;
   for(int side=0;side<sides;side++){
    float a=6.2831853f*side/sides,b=6.2831853f*(side+1)/sides;
    auto point=[&](float angle,float r,float yy){return vertex(nozzle+std::cos(angle)*r,yy,z+std::sin(angle)*r,std::cos(angle),std::sin(angle),light);};
    auto v0=point(a,radius-thickness,y),v1=point(b,radius-thickness,y),v2=point(b,radius+thickness,y-.013f),v3=point(a,radius+thickness,y-.013f);
    for(auto v:{v0,v1,v2,v0,v2,v3})out.push_back(v);
   }
  }
 }
 void append(std::vector<MeshVertex>&out,physics::Vec3 eye,float yaw,bool third)const{
  if(state==Absent||(state==Worn&&!third))return;
  float c=std::cos(yaw),s=std::sin(yaw);
  auto box=[&](float x,float y,float z,float w,float h,float d,float r,float g,float b){
   std::vector<MeshVertex> local;appendCube(local,{x,y,z,w,h,d,r,g,b,Plain});
   for(auto v:local){float px=v.x,pz=v.z,nx=v.nx,nz=v.nz;v.x=eye.x+c*px-s*pz;v.y+=eye.y;v.z=eye.z-s*px-c*pz;v.nx=c*nx-s*nz;v.nz=-s*nx-c*nz;out.push_back(v);}
  };
  float x=state==InHands?.32f:0,z=state==InHands?.8f:-.31f,y=state==InHands?-.45f:-.55f;
  box(x,y,z,.40f,.50f,.17f,.16f,.22f,.17f);
  for(float side:{-.16f,.16f}){box(x+side,y,z+.10f,.13f,.60f,.15f,.30f,.34f,.31f);box(x+side,y-.33f,z+.10f,.15f,.09f,.17f,.08f,.09f,.08f);}
 }
};
}
