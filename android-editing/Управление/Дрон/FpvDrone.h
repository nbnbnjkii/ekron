#pragma once
#include "CollisionMesh.h"
namespace sandbox {
// A single recoverable drone; separate from furniture IDs/save format.
struct FpvDrone {
 enum Mode {Absent=0,InHands=1,Ground=2,Entering=3,Flying=4,Leaving=5,Hover=6,Returning=7};
 bool crashPending=false;physics::Vec3 crashPoint{};
 Mode mode=Absent;physics::Vec3 position{},velocity{},bodyEye{},cameraStart{};
 bool linked=false;physics::Vec3 home{};int returnStage=0,returnClimbs=0;float returnBlocked=0,returnCruise=0;
 bool thirdPerson=false;physics::Vec3 chaseEye{};float exitRoll=0;
 float lookPitch=0;
 float orbitYaw=0,orbitElevation=.35f,viewMix=0;bool orbitInitialized=false;
 float viewWeight()const {float t=std::clamp(viewMix,0.f,1.f);return t*t*(3-2*t);}
 bool outsideView()const{return thirdPerson||viewMix>0;}
 float roll=0,yaw=0,pitch=0,bodyYaw=0,bodyPitch=0,exitYaw=0,exitPitch=0,transition=0,fallSpeed=0;
 static constexpr float radius=.35f;
 static constexpr float transitionSeconds=1.6f;
 bool transitionPresented=false;
 bool piloting()const{return mode==Entering||mode==Flying||mode==Leaving;}
 bool blocked(physics::Vec3 p,const physics::CollisionMesh&city,const physics::CollisionMesh&built)const{
  float height=.10f+radius*(std::abs(std::sin(pitch))+std::abs(std::sin(roll)));
  return city.intersectsBox(p,{radius,height,radius})||built.intersectsBox(p,{radius,height,radius});
 }
 bool take(physics::Vec3 eye,float angle,const physics::CollisionMesh&city,const physics::CollisionMesh&built){
  if(piloting())return false;
  auto p=eye+physics::Vec3{-std::sin(angle)*.75f,-.35f,-std::cos(angle)*.75f};
  physics::CollisionMesh::RayHit hit;auto d=p-eye;float distance=physics::length(d);
  if(blocked(p,city,built)||city.raycast(eye,d,distance,hit)||built.raycast(eye,d,distance,hit))return false;
  position=p;lookPitch=0;thirdPerson=false;viewMix=0;orbitInitialized=false;yaw=angle;pitch=roll=0;linked=false;velocity={};mode=InHands;return true;
 }
 bool place(const physics::CollisionMesh&city,const physics::CollisionMesh&built){
  if(mode!=InHands)return false;
  float floor=std::max(city.groundHeight(position.x,position.z,position.y+.1f,position.y-3),built.groundHeight(position.x,position.z,position.y+.1f,position.y-3));
  if(!std::isfinite(floor))return false;
  auto target=position;target.y=floor+.105f;
  int steps=std::max(1,int(std::ceil(physics::length(target-position)/.03f)));
  for(int i=1;i<=steps;i++)if(blocked(position+(target-position)*(float(i)/steps),city,built))return false;
  position=target;mode=Ground;fallSpeed=0;return true;
 }
 bool aimed(physics::Vec3 eye,float angle,float tilt,const physics::CollisionMesh&city,const physics::CollisionMesh&built)const{
  if(mode!=Ground&&mode!=Hover)return false;
  physics::Vec3 direction{-std::sin(angle)*std::cos(tilt),std::sin(tilt),-std::cos(angle)*std::cos(tilt)};
  auto delta=position-eye;float along=physics::dot(delta,direction);
  if(along<0||along>8||physics::length(delta-direction*along)>.35f)return false;
  physics::CollisionMesh::RayHit hit;
  return !city.raycast(eye,direction,std::max(0.f,along-radius),hit)&&!built.raycast(eye,direction,std::max(0.f,along-radius),hit);
 }
 bool connect(physics::Vec3 eye,float angle,float tilt,const physics::CollisionMesh&city,const physics::CollisionMesh&built){
  if(!(linked&&(mode==Hover||mode==Ground||mode==Returning))&&!aimed(eye,angle,tilt,city,built))return false;
  linked=true;thirdPerson=false;viewMix=0;orbitInitialized=false;
  bodyEye=eye;bodyYaw=angle;bodyPitch=tilt;transition=0;transitionPresented=false;mode=Entering;velocity={};return true;
 }
 float feedYaw()const{return yaw;}
 float feedPitch()const{return std::clamp(pitch+lookPitch,-1.3f,1.3f);}
 void look(float dx,float dy){
  if(mode!=Flying||!std::isfinite(dx)||!std::isfinite(dy))return;
  if(thirdPerson){orbitYaw=std::remainder(orbitYaw-dx*.004f,6.28318530718f);orbitElevation=std::clamp(orbitElevation-dy*.004f,-1.35f,1.35f);}
  else lookPitch=std::clamp(lookPitch-dy*.004f,-1.2f,1.2f);
 }
 void updateChase(const physics::CollisionMesh&city,const physics::CollisionMesh&built){
  auto anchor=position+physics::Vec3{0,.08f,0};
  auto desired=anchor+physics::Vec3{std::sin(orbitYaw)*std::cos(orbitElevation)*2.65f,std::sin(orbitElevation)*2.65f,std::cos(orbitYaw)*std::cos(orbitElevation)*2.65f};
  auto delta=desired-anchor;int steps=std::max(1,int(std::ceil(physics::length(delta)/.04f)));chaseEye=anchor;
  for(int i=1;i<=steps;i++){auto p=anchor+delta*(float(i)/steps);
   if(city.intersectsBox(p,{.12f,.12f,.12f})||built.intersectsBox(p,{.12f,.12f,.12f}))break;
   chaseEye=p;
  }
 }
 void toggleView(const physics::CollisionMesh&city,const physics::CollisionMesh&built){if(mode==Flying){
  thirdPerson=!thirdPerson;
  if(!orbitInitialized){orbitYaw=yaw;orbitElevation=.35f;orbitInitialized=true;}
  if(thirdPerson&&viewMix==0)updateChase(city,built);
 }}
 float targetYaw()const{return feedYaw()+std::remainder(orbitYaw-feedYaw(),6.28318530718f)*viewWeight();}
 float targetPitch()const {auto delta=position+physics::Vec3{0,.08f,0}-chaseEye;float orbitPitch=std::atan2(delta.y,std::hypot(delta.x,delta.z));return feedPitch()+(orbitPitch-feedPitch())*viewWeight();}
 float viewRoll()const {return (mode==Leaving?exitRoll:roll*(1-viewWeight()))*blend();}
 float viewYaw()const {float t=blend();if(mode==Leaving)return bodyYaw+std::remainder(exitYaw-bodyYaw,6.28318530718f)*t;return bodyYaw+std::remainder(targetYaw()-bodyYaw,6.28318530718f)*t;}
 float viewPitch()const {return bodyPitch+((mode==Leaving?exitPitch:targetPitch())-bodyPitch)*blend();}
 void leave(){if(mode==Entering||mode==Flying){cameraStart=camera();exitYaw=viewYaw();exitPitch=viewPitch();exitRoll=viewRoll();transition=0;transitionPresented=false;mode=Leaving;velocity={};}}
 physics::Vec3 feedEye()const{return position+physics::Vec3{-std::sin(yaw)*.16f,.05f,-std::cos(yaw)*.16f};}
 physics::Vec3 camera()const{
  auto droneEye=feedEye()+(chaseEye-feedEye())*viewWeight();
  float t=std::clamp(transition/transitionSeconds,0.f,1.f);t=t*t*(3-2*t);
  if(mode==Entering)return bodyEye+(droneEye-bodyEye)*t;
  if(mode==Leaving)return cameraStart+(bodyEye-cameraStart)*t;
  return droneEye;
 }
 float blend()const {float t=std::clamp(transition/transitionSeconds,0.f,1.f);t=t*t*(3-2*t);return mode==Entering?t:mode==Leaving?1-t:1;}
 bool returnHome(physics::Vec3 eye,float angle,const physics::CollisionMesh&city,const physics::CollisionMesh&built){
  if(piloting()||!(mode==Ground||mode==Hover||mode==Returning))return false;
  auto target=eye+physics::Vec3{-std::sin(angle)*.9f,-.45f,-std::cos(angle)*.9f};
  if(blocked(target,city,built))return false;
  home=target;returnStage=0;returnBlocked=0;returnClimbs=0;returnCruise=std::max(position.y,home.y)+1;velocity={};mode=Returning;linked=true;return true;
 }
 void update(float dt,float right,float forward,float vertical,float turn,float look,const physics::CollisionMesh&city,const physics::CollisionMesh&built){
  dt=std::clamp(dt,0.f,.05f);
  if(mode==Entering||mode==Leaving){if(!transitionPresented){transitionPresented=true;return;}transition+=dt;if(transition>=transitionSeconds)mode=mode==Entering?Flying:Hover;return;}
  (void)look;
  if(mode!=Flying&&mode!=Ground&&mode!=Hover&&mode!=Returning)return;
  if(mode==Flying){
   // Camera interpolation never resets flight inputs, attitude or velocity.
   viewMix=std::clamp(viewMix+(thirdPerson?1.f:-1.f)*dt/.6f,0.f,1.f);
   // Mode 2 radio layout: throttle/yaw on the left, pitch/roll on the right.
   yaw-=std::clamp(turn,-1.f,1.f)*1.8f*dt;
   float response=1-std::exp(-8*dt);
   pitch+=(-std::clamp(forward,-1.f,1.f)*.8f-pitch)*response;
   roll+=(std::clamp(right,-1.f,1.f)*.8f-roll)*response;
   float floor=std::max(city.groundHeight(position.x,position.z,position.y,position.y-.5f),built.groundHeight(position.x,position.z,position.y,position.y-.5f));
   if(std::isfinite(floor)){
    float available=std::max(0.f,position.y-floor-.101f),needed=radius*(std::abs(std::sin(pitch))+std::abs(std::sin(roll)));
    if(needed>available&&needed>1e-6f){pitch*=available/needed;roll*=available/needed;}
   }
   float side=std::sin(roll)*10,front=-std::sin(pitch)*10;
   physics::Vec3 acceleration{std::cos(yaw)*side-std::sin(yaw)*front,0,-std::sin(yaw)*side-std::cos(yaw)*front};
   velocity.x=(velocity.x+acceleration.x*dt)*std::exp(-1.2f*dt);
   velocity.z=(velocity.z+acceleration.z*dt)*std::exp(-1.2f*dt);
   velocity.y+=(std::clamp(vertical,-1.f,1.f)*3-velocity.y)*(1-std::exp(-6*dt));
  }else if(mode==Ground){fallSpeed=std::max(-12.f,fallSpeed-18*dt);velocity={0,fallSpeed,0};}
  else if(mode==Hover){velocity=velocity*std::exp(-7*dt);pitch*=std::exp(-8*dt);roll*=std::exp(-8*dt);}
  else {
   auto target=home;
   if(returnStage==0){target={position.x,returnCruise,position.z};if(position.y>=returnCruise-.03f)returnStage=1;}
   if(returnStage==1){target={home.x,returnCruise,home.z};if(std::hypot(position.x-home.x,position.z-home.z)<.08f)returnStage=2;}
   if(returnStage==2)target=home;
   auto offset=target-position;float distance=physics::length(offset);
   if(returnStage==2&&distance<.04f){mode=Hover;velocity={};return;}
   auto desired=offset*(std::min(2.f,distance*2)/std::max(.001f,distance));
   velocity=velocity+(desired-velocity)*(1-std::exp(-6*dt));pitch*=std::exp(-8*dt);roll*=std::exp(-8*dt);
  }
  auto before=position;auto delta=velocity*dt;int steps=std::max(1,int(std::ceil(physics::length(delta)/.025f)));
  for(int i=0;i<steps;i++)for(int axis=0;axis<3;axis++){
   auto p=position;if(axis==0)p.x+=delta.x/steps;if(axis==1)p.y+=delta.y/steps;if(axis==2)p.z+=delta.z/steps;
   if(!blocked(p,city,built))position=p;else {
    float impact=axis==0?std::abs(velocity.x):axis==1?std::abs(velocity.y):std::abs(velocity.z);
    if((mode==Flying||mode==Returning||mode==Hover)&&impact>=2.8f){crashPoint=position;crashPending=true;mode=Absent;linked=false;velocity={};thirdPerson=false;viewMix=0;return;}
    if(axis==0)velocity.x=0;
    if(axis==1){velocity.y=0;fallSpeed=0;}
    if(axis==2)velocity.z=0;
   }
  }
  if(mode==Returning){
   if(physics::length(position-before)<.002f)returnBlocked+=dt;else returnBlocked=0;
   // No teleporting or pushing through a facade: a blocked return safely hovers.
   if(returnBlocked>.6f){
    if(returnStage==1&&returnClimbs<8){returnCruise+=1;returnClimbs++;returnStage=0;returnBlocked=0;}
    else {mode=Hover;velocity={};}
   }
  }
 }
};
}
