#include "../../sys/android/app/src/main/cpp/CrashCamera.h"
#include <cassert>
#include <iostream>
int main(){
 physics::CollisionMesh city,built;city.addBox({-30,-1,-30},{30,0,30});city.build();built.build();
 auto p=sandbox::crashOverview({0,.3f,0},0,city,built);
 assert(physics::length(p-physics::Vec3{0,.3f,0})>6.5f&&p.y>2.5f);
 built.addBox({-2,0,1},{2,8,3});built.build();
 p=sandbox::crashOverview({0,.3f,0},0,city,built);
 assert(!built.intersectsBox(p,{.2f,.2f,.2f}));assert(physics::length(p-physics::Vec3{0,.3f,0})>5);
 auto origin=physics::Vec3{0,.6f,0};auto delta=p-origin;
 for(int i=0;i<=100;i++)assert(!built.intersectsBox(origin+delta*(i/100.f),{.19f,.19f,.19f}));
 std::cout<<"Crash overview: distant elevated view and wall-safe alternate angle OK\n";
}
