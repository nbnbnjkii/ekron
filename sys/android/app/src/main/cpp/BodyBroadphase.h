#pragma once
#include "BodyContacts.h"
#include <tuple>
#include <cstdint>
namespace sandbox {
inline void sanitizeBodyState(Object&o){
 if(!std::isfinite(o.x)||!std::isfinite(o.y)||!std::isfinite(o.z)){o.destroyed=true;o.x=o.y=o.z=0;}
 auto clean=[](float&v,float cap){v=std::isfinite(v)?std::clamp(v,-cap,cap):0.f;};
 clean(o.vx,100);clean(o.vy,100);clean(o.vz,100);clean(o.spinX,30);clean(o.spinZ,30);
 clean(o.tiltX,10000);clean(o.tiltZ,10000);clean(o.doorAngle,6.3f);clean(o.restTime,3600);
}
inline std::vector<size_t> physicalOrder(const World&w){
 std::vector<size_t> order;for(size_t i=0;i<w.objects.size();i++)if(!w.objects[i].destroyed)order.push_back(i);
 std::sort(order.begin(),order.end(),[&](size_t a,size_t b){
  auto key=[](const Object&source){Object o=source;sanitizeBodyState(o);
   return std::make_tuple(o.y,o.x,o.z,o.type,o.rotation,o.tiltX,o.tiltZ,o.vx,o.vy,o.vz,o.spinX,o.spinZ,o.doorAngle,o.frozen,o.anchored,o.awake,o.restTime);
  };
  return key(w.objects[a])<key(w.objects[b]);
 });return order;
}
class BodyBroadphase {
 struct Cached {Object pose{};physics::CollisionMesh mesh;bool valid=false;};
 std::vector<Cached> cache_;
 std::vector<Object> graphPoses_;
 int graphHeld_=-2;
public:
 std::vector<std::vector<size_t>> neighbours;
 size_t candidatePairs=0,meshBuilds=0,graphBuilds=0,graphReuses=0,gridEntries=0,oversizedBodies=0;
 void build(const World&w,int held,float dt,const std::vector<Object>*before=nullptr){
  bool unchanged=graphHeld_==held&&graphPoses_.size()==w.objects.size();
  for(size_t i=0;i<w.objects.size()&&unchanged;i++){
   const auto&o=w.objects[i];const auto&p=graphPoses_[i];
   if((o.awake&&!o.frozen&&!o.anchored)||o.type!=p.type||o.rotation!=p.rotation||o.x!=p.x||o.y!=p.y||o.z!=p.z||o.tiltX!=p.tiltX||o.tiltZ!=p.tiltZ||o.doorAngle!=p.doorAngle||o.destroyed!=p.destroyed)unchanged=false;
  }
  if(unchanged){meshBuilds=0;graphReuses++;return;}
  graphPoses_=w.objects;graphHeld_=held;graphBuilds++;
  struct Cell {int x,y,z;size_t index;};std::vector<Cell> cells;
  std::vector<size_t> globalBodies;
  // One bit per pair: never allocate duplicate neighbour entries for every
  // shared grid cell. Dense piles previously created millions of duplicates.
  const size_t count=w.objects.size();std::vector<unsigned char> pairs((count*count+7)/8);
  auto addPair=[&](size_t i,size_t j){if(i==j)return;if(i>j)std::swap(i,j);size_t bit=i*count+j;
   auto&byte=pairs[bit/8];unsigned char mask=static_cast<unsigned char>(1u<<(bit%8));if(byte&mask)return;byte|=mask;
   neighbours[i].push_back(j);neighbours[j].push_back(i);
  };
  auto order=physicalOrder(w);std::vector<size_t> rank(w.objects.size());for(size_t i=0;i<order.size();i++)rank[order[i]]=i;
  neighbours.assign(w.objects.size(),{});cache_.resize(w.objects.size());candidatePairs=0;meshBuilds=0;
  for(size_t i:order){const auto&o=w.objects[i];if(int(i)==held||o.destroyed)continue;
   physics::Bounds bounds;
   for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f}){
    auto p=bodyContactPoint(o,x,y,z);bounds.add(p);bounds.add(p+physics::Vec3{o.vx*dt,o.vy*dt,o.vz*dt});
    if(before&&i<before->size())bounds.add(bodyContactPoint((*before)[i],x,y,z));
   }
   bounds.lo=bounds.lo-physics::Vec3{.3f,.3f,.3f};bounds.hi=bounds.hi+physics::Vec3{.3f,.3f,.3f};
   auto cell=[](float p){return int(std::floor(std::clamp(p,-8192.f,8192.f)/4));};
   bool finite=true;for(float v:{bounds.lo.x,bounds.lo.y,bounds.lo.z,bounds.hi.x,bounds.hi.y,bounds.hi.z})finite=finite&&std::isfinite(v);
   if(!finite){globalBodies.push_back(i);continue;}
   int x0=cell(bounds.lo.x),x1=cell(bounds.hi.x),y0=cell(bounds.lo.y),y1=cell(bounds.hi.y),z0=cell(bounds.lo.z),z1=cell(bounds.hi.z);
   const int64_t entries=int64_t(x1-x0+1)*(y1-y0+1)*(z1-z0+1);
   // Sweeps with exceptional velocities use conservative all-body candidates,
   // not millions of cells. This bounds memory without dropping collisions.
   if(entries<1||entries>64){globalBodies.push_back(i);continue;}
   for(int x=x0;x<=x1;x++)for(int y=y0;y<=y1;y++)for(int z=z0;z<=z1;z++)cells.push_back({x,y,z,i});
  }
  std::sort(cells.begin(),cells.end(),[&](const Cell&a,const Cell&b){return std::make_tuple(a.x,a.y,a.z,rank[a.index])<std::make_tuple(b.x,b.y,b.z,rank[b.index]);});
  for(size_t first=0;first<cells.size();){size_t end=first+1;
   while(end<cells.size()&&cells[first].x==cells[end].x&&cells[first].y==cells[end].y&&cells[first].z==cells[end].z)end++;
   for(size_t a=first;a<end;a++)for(size_t b=a+1;b<end;b++){auto i=cells[a].index,j=cells[b].index;addPair(i,j);}
   first=end;
  }
  gridEntries=cells.size();oversizedBodies=globalBodies.size();
  for(size_t i:globalBodies)for(size_t j:order)if(int(j)!=held)addPair(i,j);
  for(auto&list:neighbours){std::sort(list.begin(),list.end(),[&](size_t a,size_t b){return rank[a]<rank[b];});list.erase(std::unique(list.begin(),list.end()),list.end());candidatePairs+=list.size();}
  candidatePairs/=2;
 }
 const physics::CollisionMesh&solid(const World&w,size_t i){
  auto &c=cache_[i];const auto&o=w.objects[i];const auto&p=c.pose;
  if(!c.valid||p.type!=o.type||p.rotation!=o.rotation||p.x!=o.x||p.y!=o.y||p.z!=o.z||p.tiltX!=o.tiltX||p.tiltZ!=o.tiltZ||p.doorAngle!=o.doorAngle){
   c.mesh=physics::CollisionMesh{};appendBodySolid(c.mesh,o);c.mesh.build();c.pose=o;c.valid=true;meshBuilds++;
  }return c.mesh;
 }
 bool overlaps(const World&w,size_t index,Object candidate,physics::Vec3 centre)const{
  auto b=World::bounds(candidate);candidate.x=centre.x;candidate.y=centre.y-b.h/2;candidate.z=centre.z;
  for(size_t j:neighbours[index])if(!w.objects[j].destroyed&&bodiesOverlap(candidate,w.objects[j]))return true;
  return false;
 }
};
}
