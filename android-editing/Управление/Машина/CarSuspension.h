#pragma once
#include "BodyContacts.h"
namespace sandbox {
struct SuspensionResult {int contacts=0;float force=0;};
// Four independent compression-only spring/dampers. The city remains immutable.
// Rest length .18 m, travel .24 m; k scales with sprung mass.
// The contact footprint is measured from the original model tyre bottom.
template<class Ground>
inline SuspensionResult stepSuspension(Object&o,float dt,Ground ground,bool apply=true){
 SuspensionResult result;
 if(o.type!=8)return result;
 if(o.frozen||o.anchored||o.destroyed||std::cos(o.tiltX)*std::cos(o.tiltZ)<.55f){
  for(auto&wheel:o.wheels){wheel.load=0;wheel.grounded=false;}return result;
 }
 const float mass=World::mass(o),k=mass*18.f/(4*.09f),damper=1.6f*std::sqrt(k*mass/4);
 auto b=World::bounds(o);float ix=mass*(b.h*b.h+b.d*b.d)/12,iz=mass*(b.w*b.w+b.h*b.h)/12;
 float torqueX=0,torqueZ=0;int i=0;
 for(float x:{-.65f,.65f})for(float z:{-.72f,.72f}){
  auto point=objectCorner(o,x,-1,z);auto &wheel=o.wheels[i++];
  float h=ground(point.x,point.z,point.y+.22f,point.y-.24f);
  wheel.grounded=std::isfinite(h)&&point.y-h<=.18f&&point.y-h>=-.24f;
  wheel.compression=wheel.grounded?std::clamp(.18f-(point.y-h),0.f,.24f):0;
  wheel.load=0;
  if(!wheel.grounded)continue;
  result.contacts++;
  float rx=point.x-b.x,rz=point.z-b.z;
  float speed=o.vy+o.spinZ*rx-o.spinX*rz;
  wheel.load=std::clamp(k*wheel.compression-damper*speed,0.f,mass*18.f*.8f);
  result.force+=wheel.load;torqueX-=rz*wheel.load;torqueZ+=rx*wheel.load;
 }
 if(apply){o.vy+=result.force/mass*dt;o.spinX+=torqueX/ix*dt;o.spinZ+=torqueZ/iz*dt;}
 return result;
}
}
