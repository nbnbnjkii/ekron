#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include <cassert>
#include <limits>
#include <iostream>
int main(){
 using namespace sandbox;
 World w;w.objects.clear();
 for(int i=0;i<500;i++){Object o{12,0,float(i),0};o.vx=o.vy=o.vz=1e8f;w.objects.push_back(o);}
 BodyBroadphase grid;grid.build(w,-1,.05f);
 assert(grid.gridEntries<=64*w.objects.size()&&grid.oversizedBodies==500);
 assert(grid.candidatePairs==500*499/2);
 for(const auto&list:grid.neighbours)assert(list.size()==499);
 // Kilometre-wide maps must not collapse all distant objects into edge cells.
 w.objects={{12,0,300,300},{12,0,900,900}};grid.build(w,-1,.01f);assert(grid.candidatePairs==0);
 // Duplicate grid cells produce exactly one neighbour pair.
 w.objects.assign(500,Object{1,0,0,0});grid.build(w,-1,.01f);assert(grid.candidatePairs==500*499/2);
 for(const auto&list:grid.neighbours)assert(list.size()==499);
 // Invalid sort keys and velocities must be repaired before spatial queries.
 w.objects={{12,0,0,0},{12,0,2,0}};
 w.objects[0].vx=std::numeric_limits<float>::infinity();w.objects[0].tiltX=std::numeric_limits<float>::quiet_NaN();w.objects[1].spinZ=std::numeric_limits<float>::quiet_NaN();
 assert(physicalOrder(w).size()==2);
 physics::CollisionMesh floor;floor.addBox({-20,-.2f,-20},{20,0,20});floor.build();ObjectDynamics dynamics;
 for(int i=0;i<120;i++)dynamics.update(w,floor,.016f,-1,{-20,-5,-20},{20,20,20});
 for(const auto&o:w.objects)assert(std::isfinite(o.x)&&std::isfinite(o.y)&&std::isfinite(o.vx)&&std::isfinite(o.tiltX)&&std::isfinite(o.spinZ));
 std::cout<<"Broadphase safety: 500 extreme sweeps/dense bodies bounded, unique pairs, distant map cells, NaN/Inf repair OK\n";
}
