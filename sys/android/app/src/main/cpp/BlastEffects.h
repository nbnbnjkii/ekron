#pragma once
#include "CollisionMesh.h"
#include "MeshGeometry.h"
namespace sandbox {
struct Fragment {physics::Vec3 p,v;float life=0,size=.06f;int kind=0;}; // kind: 0 ember, 1 smoke
struct Flash {physics::Vec3 p;float age=0;};
class BlastEffects {
 std::vector<Fragment> fragments_;
 std::vector<Flash> flashes_;
public:
 std::array<float,4> light()const{if(flashes_.empty())return {};const auto&f=flashes_.back();return {f.p.x,f.p.y,f.p.z,std::max(0.f,1-f.age/.4f)};}
 size_t fragmentCount()const{return fragments_.size();}
 void explode(World&w,physics::Vec3 point){
  if(flashes_.size()>=16)flashes_.erase(flashes_.begin());
  flashes_.push_back({point,0});
  // Bouncing embers plus slower growing smoke instead of identical bright cubes.
  auto addFragment=[&](const Fragment&f){
   if(fragments_.size()>=128)fragments_.erase(fragments_.begin());
   fragments_.push_back(f);
  };
  for(int i=0;i<24;i++){float a=i*2.399963229728653f;float speed=2.2f+float(i%4)*1.1f;
   addFragment(Fragment{point,{std::sin(a)*speed,2.6f+float(i%5)*.7f,std::cos(a)*speed},1.f+float(i%4)*.3f,.05f+.02f*(i%3),0});
  }
  for(int i=0;i<12;i++){float a=i*6.2831853f/12;
   addFragment(Fragment{point,{std::sin(a)*1.5f,.7f+float(i%3)*.5f,std::cos(a)*1.5f},2.4f,.16f+float(i%3)*.07f,1});
  }
  for(auto&o:w.objects){if(o.destroyed||o.frozen||o.anchored)continue;auto b=World::bounds(o);auto d=physics::Vec3{b.x,b.y,b.z}-point;float distance=physics::length(d);if(distance>6)continue;
   auto direction=physics::normalized(d+physics::Vec3{0,.4f,0});float speed=std::min(12.f,120/std::sqrt(World::mass(o)))*(1-distance/6);o.vx+=direction.x*speed;o.vy+=direction.y*speed+speed*.25f;o.vz+=direction.z*speed;o.awake=true;o.restTime=0;}
 }
 void update(float dt,const physics::CollisionMesh&city,const physics::CollisionMesh&built){
  dt=std::clamp(dt,0.f,.05f);
  for(auto&f:flashes_)f.age+=dt;
  flashes_.erase(std::remove_if(flashes_.begin(),flashes_.end(),[](const Flash&f){return f.age>.45f;}),flashes_.end());
  for(auto&f:fragments_){f.life-=dt;if(f.kind==1)f.size+=dt*.55f;f.v.y-=18*dt*.4f;auto delta=f.v*dt;int steps=std::max(1,int(std::ceil(physics::length(delta)/.025f)));
   for(int i=0;i<steps;i++){auto next=f.p+delta*(1.f/steps);if(city.intersectsBox(next,{.05f,.05f,.05f})||built.intersectsBox(next,{.05f,.05f,.05f})){f.v=f.v*-.18f;break;}f.p=next;}}
  fragments_.erase(std::remove_if(fragments_.begin(),fragments_.end(),[](const Fragment&f){return f.life<=0;}),fragments_.end());
 }
 void append(std::vector<MeshVertex>&v)const{
  for(const auto&f:fragments_){
   if(f.kind==1){float t=std::clamp(f.life/2.4f,0.f,1.f);appendSphere(v,f.p.x,f.p.y,f.p.z,f.size,.30f+.10f*t,.31f+.10f*t,.34f+.10f*t,Plain,5,10);}
   else{float glow=std::clamp(f.life/1.5f,0.f,1.f);appendSphere(v,f.p.x,f.p.y,f.p.z,f.size,.95f,.45f*glow+.06f,.06f*glow,Plain,4,8);}
  }
  for(const auto&f:flashes_){float t=std::clamp(f.age/.45f,0.f,1.f);float s=.4f+std::sin(t*3.14159265f)*2.4f;
   appendSphere(v,f.p.x,f.p.y,f.p.z,s,1.f,.8f*(1-t)+.3f*t,.15f*(1-t),Plain,6,12);}
 }
};
}
