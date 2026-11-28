#pragma once
#include "MeshGeometry.h"
#include "PlayerPhysics.h"
#include "LobbyDecor.h"
#include <array>
namespace sandbox {
class LobbyScene {
 physics::CollisionMesh collision_;
 physics::PlayerController player_;
public:
 float x=0,y=1.7f,z=-3,yaw=0,pitch=0;bool crouched=false;
 std::vector<MeshVertex> room,effects;
 float clock=0;
 static physics::Vec3 screenPoint(int screen,float u,float v){
  float across=(u-.5f)*6.4f,yy=3.9f-v*3.6f;
  switch(screen){case 0:return {across,yy,-7.94f};case 1:return {7.94f,yy,across};case 2:return {-across,yy,7.94f};default:return {-7.94f,yy,-across};}
 }
 LobbyScene(){
  LobbyDecor{room,collision_}.build();
  collision_.build();player_.setBoundary(-8,8,-8,8);player_.reset(x,y,z);
 }
 void update(float right,float forward,float dx,float dy,bool jump,bool crouch,float dt){
  clock=std::fmod(clock+std::clamp(dt,0.f,.05f),3600.f);effects.clear();LobbyDecor{effects,collision_}.hologram(clock);
  yaw-=std::clamp(dx,-400.f,400.f)*.004f;pitch=std::clamp(pitch-dy*.004f,-1.3f,1.3f);crouched=crouch;
  if(jump)player_.jump();
  player_.update(collision_,right,forward,yaw,dt,crouched,x,y,z);
 }
 physics::Vec3 eye()const{return {x,y,z};}
 physics::Vec3 direction(float nx,float ny,float aspect)const{
  physics::Vec3 q{(2*nx-1)*aspect/1.37638f,(1-2*ny)/1.37638f,-1};
  float cy=std::cos(yaw),sy=std::sin(yaw),cp=std::cos(pitch),sp=std::sin(pitch);
  float yy=cp*q.y-sp*q.z,zz=sp*q.y+cp*q.z;
  return physics::normalized({cy*q.x+sy*zz,yy,-sy*q.x+cy*zz});
 }
 std::array<float,3> hit(float nx,float ny,float aspect)const{
  auto d=direction(nx,ny,aspect);float closest=1e9f;std::array<float,3> result{-1,0,0};
  for(int i=0;i<4;i++){
   auto a=screenPoint(i,0,0),u=screenPoint(i,1,0)-a,v=screenPoint(i,0,1)-a,n=physics::cross(u,v);
   float denominator=physics::dot(d,n);if(std::abs(denominator)<1e-6f)continue;
   float t=physics::dot(a-eye(),n)/denominator;if(t<=0||t>=closest)continue;
   auto p=eye()+d*t-a;float su=physics::dot(p,u)/physics::dot(u,u),sv=physics::dot(p,v)/physics::dot(v,v);
   if(su>=0&&su<=1&&sv>=0&&sv<=1){closest=t;result={float(i),su,sv};}
  }return result;
 }
 // Same projection as the GLES shader; used only to position the video overlay.
 bool project(physics::Vec3 p,float aspect,float&nx,float&ny)const{
  p=p-eye();float cy=std::cos(yaw),sy=std::sin(yaw),cp=std::cos(pitch),sp=std::sin(pitch);
  float xx=cy*p.x-sy*p.z,zz=sy*p.x+cy*p.z,yy=cp*p.y+sp*zz;zz=-sp*p.y+cp*zz;
  if(zz>=-.1f)return false;
  nx=(xx*1.37638f/(-zz*aspect)+1)*.5f;ny=(1-yy*1.37638f/-zz)*.5f;return true;
 }
};
}
