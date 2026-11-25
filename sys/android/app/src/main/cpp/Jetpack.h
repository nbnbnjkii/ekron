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
 void remove(){state=Absent;}
 bool thrustAllowed(bool seated,bool piloting,bool crouched,bool prone)const{return state==Worn&&!seated&&!piloting&&!crouched&&!prone;}
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
