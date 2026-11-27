#pragma once
#include "SandboxWorld.h"
#include "CollisionMesh.h"
#include "ObjectOverlap.h"
#include "BodyContacts.h"
#include <optional>
namespace sandbox {
enum class HandTool { HAND_EMPTY=0, SPAWN_GUN=1, GRAVITY_GUN=2, DELETE_TOOL=3, DRONE_REMOTE=4 };
class ToolInteraction {
public:
 HandTool tool=HandTool::HAND_EMPTY;
 int held=-1;
 float holdDistance=3.5f;
 void adjustDistance(float delta){if(std::isfinite(delta))holdDistance=std::clamp(holdDistance+delta,1.5f,12.f); }
 physics::Vec3 display{};
 Object original{};
 bool holding()const{return held>=0;}
 void cancel(World&w){if(holding()&&size_t(held)<w.objects.size())w.objects[held]=original;held=-1;}
 void select(World&w,HandTool next){cancel(w);tool=next;}
 bool grab(World&w,const physics::CollisionMesh*city){
  if(tool!=HandTool::GRAVITY_GUN||holding())return false;
  physics::Vec3 origin{w.x,w.y,w.z},dir{-std::sin(w.yaw)*std::cos(w.pitch),std::sin(w.pitch),-std::cos(w.yaw)*std::cos(w.pitch)};
  float nearest=8;int found=-1;
  for(size_t i=0;i<w.objects.size();i++){
   float hit=World::rayDistance(w.objects[i],origin.x,origin.y,origin.z,dir.x,dir.y,dir.z,nearest);
   if(hit<nearest){nearest=hit;found=int(i);}
  }
  if(found<0)return false;
  // Reject a sight ray hidden by solid city geometry (including the alpha fence BVH).
  if(city){physics::CollisionMesh::RayHit hit;if(city->raycast(origin,dir,std::max(0.f,nearest-.015f),hit))return false;}
  held=found;original=w.objects[held];w.objects[held].awake=false;w.objects[held].vx=w.objects[held].vy=w.objects[held].vz=w.objects[held].spinX=w.objects[held].spinZ=0;auto b=World::bounds(original);display={b.x,b.y,b.z};return true;
 }
 void pull(const World&w,float dt,const physics::CollisionMesh*mesh=nullptr){
  if(!holding())return;
  physics::Vec3 target{w.x-std::sin(w.yaw)*holdDistance,w.y+std::sin(w.pitch)*2,w.z-std::cos(w.yaw)*holdDistance};
  auto next=display+(target-display)*(1-std::exp(-8*dt));
  if(mesh){
   const auto&o=w.objects[held];
   // Lift the target enough to clear the ground even for the long UAZ body.
   auto b=World::bounds(o);float r=std::abs(std::sin(o.tiltZ))*b.w/2+std::abs(std::cos(o.tiltZ)*std::cos(o.tiltX))*b.h/2+std::abs(std::cos(o.tiltZ)*std::sin(o.tiltX))*b.d/2;
   float ground=mesh->groundHeight(next.x,next.z,display.y+.3f,display.y-r-.8f);
   if(std::isfinite(ground))next.y=std::max(next.y,ground+r+.025f);
   int steps=std::max(1,int(std::ceil(physics::length(next-display)/.04f)));
   auto start=display;
   for(int i=1;i<=steps;i++){
    auto point=start+(next-start)*(float(i)/steps);
    auto blocked=[&](physics::Vec3 p){return objectOverlap(*mesh,o,p)||overlapsBodies(w,o,held,held,p);};
    if(!blocked(point)){display=point;continue;}
    // Keep tangential progress when the normal component is obstructed.
    // Re-evaluate from the current camera target every frame, never from a
    // stale grab ray or a distance-dependent activation radius.
    auto delta=point-display;
    for(int axis:{1,0,2}){auto slide=display;if(axis==0)slide.x+=delta.x;if(axis==1)slide.y+=delta.y;if(axis==2)slide.z+=delta.z;if(!blocked(slide))display=slide;}
   }
   return;
  }
  display=next;
 }
 void rotate(World&w,const physics::CollisionMesh*mesh=nullptr){
  if(!holding())return;
  Object candidate=w.objects[held];candidate.rotation=(candidate.rotation+1)%4;
  if((mesh&&objectOverlap(*mesh,candidate,display))||overlapsBodies(w,candidate,held,held,display))return;
  w.objects[held]=candidate;
 }
 bool release(World&w){
  if(!holding())return false;
  auto &o=w.objects[held];auto b=World::bounds(o);o.x=display.x;o.z=display.z;o.y=display.y-b.h/2;
  float mass=World::mass(o);
  float impulse=std::min(16.f,std::sqrt(150000.f/mass));
  o.vx=-std::sin(w.yaw)*std::cos(w.pitch)*impulse;o.vz=-std::cos(w.yaw)*std::cos(w.pitch)*impulse;o.vy=std::sin(w.pitch)*impulse+2;
  // Off-centre throw supplies angular impulse; no X/Z rotation locks.
  float inertiaX=mass*(b.h*b.h+b.d*b.d)/12,inertiaZ=mass*(b.w*b.w+b.h*b.h)/12;
  o.spinX=o.type==8?0:std::clamp(-o.vz*mass*.12f/inertiaX,-4.f,4.f);
  o.spinZ=o.type==8?0:std::clamp(impulse*mass*.08f/inertiaZ,-4.f,4.f);
  o.embeddedTime=0;o.restTime=0;
  o.frozen=false;o.awake=true;o.anchored=false;held=-1;return true;
 }
 bool freeze(World&w,const physics::CollisionMesh*city){
  int index=holding()?held:target(w,city);if(index<0)return false;
  auto&o=w.objects[index];
  if(holding()){auto b=World::bounds(o);o.x=display.x;o.z=display.z;o.y=display.y-b.h/2;held=-1;}
  o.frozen=!o.frozen;for(auto&wheel:o.wheels)wheel.load=0;o.vx=o.vy=o.vz=o.spinX=o.spinZ=0;o.awake=!o.frozen;o.restTime=0;o.supportCheckTime=0;o.embeddedTime=0;
  if(!o.frozen)o.anchored=false;
  return true;
 }
 int target(World&w,const physics::CollisionMesh*city){
  if(tool!=HandTool::GRAVITY_GUN||holding())return -1;
  if(!grab(w,city))return -1;
  int result=held;w.objects[held]=original;held=-1;return result;
 }
};
}
