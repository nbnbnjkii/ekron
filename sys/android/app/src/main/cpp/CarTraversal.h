#pragma once
#include "ObjectCcd.h"
namespace sandbox {
// Bounded off-road step assist. Requires tyres on real terrain and sweeps both
// the lift and the raised chassis path; never disables collision or fills pits.
template<class Ground,class Blocked>
bool tryCarTerrainStep(Object&o,float dx,float dz,Ground ground,Blocked blocked,const physics::CollisionMesh*terrain=nullptr){
 if(o.type!=8||o.frozen||o.anchored||o.destroyed||std::abs(o.tiltX)>.4f||std::abs(o.tiltZ)>.4f)return false;
 float distance=std::hypot(dx,dz);if(distance<.0001f||distance>.5f)return false;
 if(!blocked(o.x+dx,o.y,o.z+dz))return false;
 constexpr float maxRise=.45f;
 float baseline=1e9f;int contacts=0;
 for(float x:{-.65f,.65f})for(float z:{-.72f,.72f}){
  auto p=objectCorner(o,x,-1,z);float h=ground(p.x,p.z,p.y+.06f,p.y-.26f);
  if(std::isfinite(h)){baseline=std::min(baseline,h);contacts++;}
 }
 if(contacts<2)return false;
 float obstacle=-1e9f;
 // Include the leading edge as well as the wheel footprint, so a square kerb
 // is detected before the front bumper can cancel all drive velocity.
 for(float x:{-1.f,0.f,1.f})for(float z:{-1.f,0.f,1.f}){
  auto p=objectCorner(o,x,-1,z);float h=ground(p.x+dx+dx/distance*.10f,p.z+dz+dz/distance*.10f,baseline+maxRise,baseline+.025f);
  if(std::isfinite(h))obstacle=std::max(obstacle,h);
 }
 // Sparse wheel/corner rays miss small stones between the nine probes.
 // Query actual nearby city triangles so narrow off-centre rocks count too.
 if(terrain){
  physics::Bounds area;
  for(float x:{-1.f,1.f})for(float z:{-1.f,1.f}){auto p=objectCorner(o,x,-1,z);area.add({p.x+dx,baseline+.025f,p.z+dz});area.add({p.x+dx,baseline+maxRise,p.z+dz});}
  terrain->query(area,[&](const physics::Triangle&t){
   auto sample=[&](physics::Vec3 p){if(p.x<area.lo.x||p.x>area.hi.x||p.z<area.lo.z||p.z>area.hi.z)return;
    float h=ground(p.x,p.z,baseline+maxRise,baseline+.025f);if(std::isfinite(h))obstacle=std::max(obstacle,h);};
   sample((t.a+t.b+t.c)*(1.f/3));sample(t.a);sample(t.b);sample(t.c);
  });
 }
 if(obstacle<baseline+.025f||obstacle>baseline+maxRise)return false;
 for(float lift=.03f;lift<=maxRise+.001f;lift+=.03f){
  if(blocked(o.x+dx,o.y+lift,o.z+dz))continue;
  bool clear=true;
  for(float up=.03f;up<=lift+.001f;up+=.03f)if(blocked(o.x,o.y+up,o.z)){clear=false;break;}
  int slices=std::max(1,int(std::ceil(distance/.03f)));
  for(int i=1;clear&&i<=slices;i++)if(blocked(o.x+dx*i/slices,o.y+lift,o.z+dz*i/slices))clear=false;
  if(!clear)continue;
  o.y+=lift;o.vy=std::max(0.f,o.vy);o.awake=true;o.restTime=0;return true;
 }
 return false;
}
}
