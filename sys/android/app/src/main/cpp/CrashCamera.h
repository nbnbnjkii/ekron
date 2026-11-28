#pragma once
#include "CollisionMesh.h"
namespace sandbox {
// Find a clear overview of the blast rather than keeping the camera inside it.
inline physics::Vec3 crashOverview(physics::Vec3 point,float yaw,const physics::CollisionMesh&city,const physics::CollisionMesh&built){
 auto blocked=[&](physics::Vec3 p){return city.intersectsBox(p,{.20f,.20f,.20f})||built.intersectsBox(p,{.20f,.20f,.20f});};
 physics::Vec3 best=point;float bestScore=-1;
 for(int i=0;i<8;i++){
  float angle=yaw+i*.7853981634f;
  physics::Vec3 delta{std::sin(angle)*7.f,3.f,std::cos(angle)*7.f};
  // Impact can be right against a wall: find a safe seed on each side first.
  auto origin=point+physics::Vec3{0,.3f,0};bool found=false;
  for(int j=0;j<=10;j++){auto seed=origin+physics::normalized(delta)*(.1f*j);if(!blocked(seed)){origin=seed;found=true;break;}}
  if(!found)continue;
  auto safe=origin;
  for(int j=1;j<=100;j++){auto p=origin+delta*(j/100.f);if(blocked(p))break;safe=p;}
  // Prefer distance, slightly favouring the original approach direction.
  float score=physics::length(safe-point)-i*.015f;
  if(score>bestScore){best=safe;bestScore=score;}
 }
 return best;
}
}
