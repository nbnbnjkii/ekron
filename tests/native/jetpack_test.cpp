#include "../../sys/android/app/src/main/cpp/Jetpack.h"
#include "../../sys/android/app/src/main/cpp/PlayerPhysics.h"
#include <cassert>
#include <iostream>
int main(){
 sandbox::Jetpack j;assert(!j.wear());assert(j.take());assert(j.wear());assert(!j.take());
 assert(j.thrustAllowed(false,false,false,false));assert(!j.thrustAllowed(true,false,false,false));assert(!j.thrustAllowed(false,true,false,false));assert(!j.thrustAllowed(false,false,true,false));
 std::vector<sandbox::MeshVertex> v;j.append(v,{0,2,0},0,false);assert(v.empty());j.append(v,{0,2,0},1,true);assert(!v.empty());for(auto a:v)assert(std::isfinite(a.x)&&std::isfinite(a.nx));
 physics::CollisionMesh scene;scene.addBox({-20,-1,-20},{20,0,20});scene.addBox({-20,4,-20},{20,5,20});scene.build();
 physics::PlayerController p;float x=0,y=1.7f,z=0;p.reset(x,y,z);
 for(int i=0;i<240;i++)p.update(scene,0,0,0,1.f/120,false,x,y,z,nullptr,false,nullptr,false,true);
 assert(y>3&&y<4.01f); // Ceiling remains solid under continuous thrust.
 for(int i=0;i<360;i++)p.update(scene,0,0,0,1.f/120,false,x,y,z);
 assert(y>1.68f&&y<1.72f); // Releasing thrust restores gravity.
 j.remove();assert(!j.thrustAllowed(false,false,false,false));
 std::cout<<"Jetpack equipment, mesh, thrust, ceiling and gravity OK\n";
}
