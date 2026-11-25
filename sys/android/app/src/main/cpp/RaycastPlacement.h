#pragma once
#include "SandboxWorld.h"
#include "CollisionMesh.h"
#include <optional>
namespace sandbox {
class RaycastPlacement {
public:
 static std::optional<Object> commitPreview(const World&w,const std::optional<Object>&shown,const physics::CollisionMesh&city,const physics::CollisionMesh&built,const std::array<float,3>&lo,const std::array<float,3>&hi){
  if(!shown||shown->type!=w.selected||shown->rotation!=w.rotation||w.objects.size()>=World::limit||!valid(w,*shown,city,built,lo,hi))return {};
  return shown; // Preserve the displayed transform, do not raycast a new one on tap.
 }
 static bool valid(const World&w,const Object&o,const physics::CollisionMesh&city,const physics::CollisionMesh&built,const std::array<float,3>&lo,const std::array<float,3>&hi){
   if(o.type<0||o.type>=kObjectTypeCount||!std::isfinite(o.y)||!std::isfinite(o.x)||!std::isfinite(o.z))return false;
  auto b=World::bounds(o);physics::Vec3 center{b.x,b.y,b.z},half{b.w/2,b.h/2,b.d/2};
  if(b.x-half.x<lo[0]||b.x+half.x>hi[0]||b.z-half.z<lo[2]||b.z+half.z>hi[2]||o.y<lo[1]-.1f||o.y+b.h>hi[1]+5)return false;
  // Indoor placement uses actual capsule clearance, not the old blanket two-metre exclusion.
  float dx=std::max(0.f,std::abs(w.x-b.x)-half.x),dz=std::max(0.f,std::abs(w.z-b.z)-half.z);
  float segmentLo=w.y-1.45f,segmentHi=w.y-.1f;
  float dy=std::max({0.f,b.y-half.y-segmentHi,segmentLo-(b.y+half.y)});
  if(dx*dx+dy*dy+dz*dz<.33f*.33f)return false;
  for(const auto&existing:w.objects){if(existing.frozen)continue;auto other=World::bounds(existing);
   if(std::abs(b.x-other.x)<(b.w+other.w)/2-.0003f&&std::abs(b.y-other.y)<(b.h+other.h)/2-.0003f&&std::abs(b.z-other.z)<(b.d+other.d)/2-.0003f)return false;
  }
  physics::Vec3 inset=half-physics::Vec3{.0003f,.0003f,.0003f};
  return !city.intersectsBox(center,inset)&&!built.intersectsBox(center,inset);
 }
 static std::optional<Object> preview(const World&w,const physics::CollisionMesh&city,const physics::CollisionMesh&built,const std::array<float,3>&lo,const std::array<float,3>&hi){
  physics::Vec3 origin{w.x,w.y,w.z},dir{-std::sin(w.yaw)*std::cos(w.pitch),std::sin(w.pitch),-std::cos(w.yaw)*std::cos(w.pitch)};
  physics::CollisionMesh::RayHit hit{},other{};
  bool found=city.raycast(origin,dir,20,hit);
  if(built.raycast(origin,dir,found?hit.distance:20,other)){hit=other;found=true;}
  if(!found||hit.distance<.35f||w.objects.size()>=World::limit)return {};
  Object candidate{w.selected,w.rotation,0,0};candidate.yaw=candidate.type==8?candidate.rotation*1.57079632679f:0.f;auto b=World::bounds(candidate);
  float extent=std::abs(hit.normal.x)*b.w/2+std::abs(hit.normal.y)*b.h/2+std::abs(hit.normal.z)*b.d/2;
  auto center=hit.position+hit.normal*(extent+.0005f);
  candidate.x=center.x;candidate.z=center.z;candidate.y=center.y-b.h/2;
  // Floor orientation is world-horizontal, never aligned to a wall hit normal.
  if(candidate.type==1){candidate.tiltX=0;candidate.tiltZ=0;}
  candidate.anchored=candidate.type!=8;candidate.surface=World::fixedSurface(candidate.type);
  if(!valid(w,candidate,city,built,lo,hi)){
   // Slide the box ALONG the hit plane to fit under ceilings/next to corners.
   // Candidate shifts come from actual nearby surfaces, not navigation strips or the y=0 plane.
   auto bounds=World::bounds(candidate);
   physics::Vec3 half{bounds.w/2,bounds.h/2,bounds.d/2},center{bounds.x,bounds.y,bounds.z};
   std::array<std::vector<float>,3> shifts;for(auto &axis:shifts)axis.push_back(0);
   auto feature=[&](physics::Vec3 point){
    float p[3]={point.x,point.y,point.z},c[3]={center.x,center.y,center.z},h[3]={half.x,half.y,half.z};
    for(int i=0;i<3;i++)for(float side:{-1.f,1.f}){
     float delta=p[i]+side*(h[i]+.0005f)-c[i];if(std::abs(delta)<.751f)shifts[i].push_back(delta);
    }
   };
   physics::Bounds nearby;nearby.add(center-half-physics::Vec3{.75f,.75f,.75f});nearby.add(center+half+physics::Vec3{.75f,.75f,.75f});
   auto collect=[&](const physics::Triangle&t){feature(t.a);feature(t.b);feature(t.c);};city.query(nearby,collect);built.query(nearby,collect);
   int fixed=std::abs(hit.normal.x)>std::abs(hit.normal.y)?0:1;
   if(std::abs(hit.normal.z)>(fixed==0?std::abs(hit.normal.x):std::abs(hit.normal.y)))fixed=2;
   for(int i=0;i<3;i++){
    auto &values=shifts[i];if(i==fixed){values={0};continue;}
    std::sort(values.begin(),values.end(),[](float a,float b){return std::abs(a)<std::abs(b);});
    std::vector<float> unique;for(float value:values){bool duplicate=false;for(float v:unique)if(std::abs(v-value)<.001f)duplicate=true;if(!duplicate)unique.push_back(value);if(unique.size()>=9)break;}values=std::move(unique);
   }
   std::vector<physics::Vec3> offsets;
   for(float x:shifts[0])for(float y:shifts[1])for(float z:shifts[2]){
    physics::Vec3 delta{x,y,z};if(std::abs(physics::dot(delta,hit.normal))<.001f)offsets.push_back(delta);
   }
   std::sort(offsets.begin(),offsets.end(),[](auto a,auto b){return physics::dot(a,a)<physics::dot(b,b);});
   bool foundFit=false;
   for(auto delta:offsets){Object adjusted=candidate;adjusted.x+=delta.x;adjusted.y+=delta.y;adjusted.z+=delta.z;
    if(valid(w,adjusted,city,built,lo,hi)){candidate=adjusted;foundFit=true;break;}}
   // A small normal correction is still useful for rough/alpha-tested surfaces, but never deep penetration.
   if(!foundFit)for(float push=.005f;push<=.05f;push+=.005f){
    Object adjusted=candidate;adjusted.x+=hit.normal.x*push;adjusted.y+=hit.normal.y*push;adjusted.z+=hit.normal.z*push;
    if(valid(w,adjusted,city,built,lo,hi)){candidate=adjusted;foundFit=true;break;}
   }
   if(!foundFit)return {};
  }
  if(hit.normal.y>.64f){
   candidate.y+=.5f;candidate.anchored=false;
   if(!valid(w,candidate,city,built,lo,hi))return {};
  }
  return candidate;
 }
};
}
