#pragma once
#include "MeshGeometry.h"
#include "PlayerPhysics.h"
#include <array>
namespace sandbox {
class LobbyScene {
 physics::CollisionMesh collision_;
 physics::PlayerController player_;
public:
 float x=0,y=1.7f,z=-3,yaw=0,pitch=0;bool crouched=false;
 std::vector<MeshVertex> room;
 static physics::Vec3 screenPoint(int screen,float u,float v){
  float across=(u-.5f)*6.4f,yy=3.9f-v*3.6f;
  switch(screen){case 0:return {across,yy,-7.94f};case 1:return {7.94f,yy,across};case 2:return {-across,yy,7.94f};default:return {-7.94f,yy,-across};}
 }
 LobbyScene(){
  auto box=[&](Box b){appendCube(room,b);collision_.addBox({b.x-b.w/2,b.y-b.h/2,b.z-b.d/2},{b.x+b.w/2,b.y+b.h/2,b.z+b.d/2});};
  box({0,-.2f,0,16,.4f,16,.23f,.26f,.21f,Plain});
  for(int i=0;i<4;i++){
   bool side=i%2;float pos=i<2?-8.15f:8.15f;
   box({side?pos:0,2.5f,side?0:pos,side?.3f:16.6f,5,side?16.6f:.3f,.17f,.22f,.18f,Plain});
  }
  // Ceiling, steel beams and real colliding military supply crates.
  box({0,5.2f,0,16.6f,.4f,16.6f,.12f,.16f,.14f,Plain});
  for(float side:{-1.f,1.f}){box({side*6.8f,.55f,5.5f,1.3f,1.1f,1.3f,.25f,.30f,.17f,Plain});box({side*6.8f,1.25f,5.5f,1.f,.3f,1.f,.19f,.24f,.13f,Plain});}
  for(int i=-6;i<=6;i+=4)appendCube(room,{float(i),4.9f,0,.14f,.2f,16,.08f,.11f,.10f,Plain});
  collision_.build();player_.setBoundary(-8,8,-8,8);player_.reset(x,y,z);
 }
 void update(float right,float forward,float dx,float dy,bool jump,bool crouch,float dt){
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
