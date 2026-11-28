#include "../../sys/android/app/src/main/cpp/Jetpack.h"
#include "../../sys/android/app/src/main/cpp/PlayerPhysics.h"
#include "../../sys/android/app/src/main/cpp/Interaction.h"
#include <cassert>
#include <iostream>
int main(){
 sandbox::Jetpack j;assert(!j.wear());assert(j.take());assert(j.wear());assert(!j.take());
 assert(j.thrustAllowed(false,false,false,false));assert(!j.thrustAllowed(true,false,false,false));assert(!j.thrustAllowed(false,true,false,false));assert(!j.thrustAllowed(false,false,true,false));assert(!j.thrustAllowed(false,false,false,true));
 std::vector<sandbox::MeshVertex> v;j.append(v,{0,2,0},0,false);assert(v.empty());j.append(v,{0,2,0},1,true);assert(!v.empty());
 v.clear();j.update(.05f,true,1);j.appendExhaust(v,{0,3,0},0);assert(!v.empty());float first=v.front().y;
 j.update(.05f,true,1);std::vector<sandbox::MeshVertex> next;j.appendExhaust(next,{0,3,0},0);assert(next.front().y!=first);
 for(auto a:next)assert(std::isfinite(a.x)&&std::isfinite(a.nx));
 physics::CollisionMesh scene;scene.addBox({-20,-1,-20},{20,0,20});scene.addBox({-20,4,-20},{20,5,20});scene.addBox({-20,0,-5},{20,4,-4.8f});scene.build();
 physics::PlayerController p;float x=0,y=1.7f,z=0;p.reset(x,y,z);
 auto fly=[&](float r,float f,float pitch,int count){for(int i=0;i<count;i++)p.update(scene,r,f,0,1.f/120,false,x,y,z,nullptr,false,nullptr,false,true,pitch);};
 fly(0,1,1.3f,240);assert(y>3.8f&&y<3.91f); // Capsule hits ceiling, not noclip.
 float height=y;fly(0,0,0,240);assert(std::abs(y-height)<.015f); // Hover, no vertical buttons.
 fly(0,1,-1.3f,240);assert(y>1.68f&&y<1.72f); // Looking down respects floor.
 p.reset(0,2.5f,0);x=0;y=2.5f;z=0;fly(0,1,0,240);assert(z>-4.56f&&z<-4.4f); // Wall remains solid.
 p.reset(0,2.5f,0);x=0;y=2.5f;z=0;fly(0,-1,.4f,15);assert(z>.5f&&y<2.49f); // Reverse follows inverse look direction.
 p.reset(0,2.5f,0);x=0;y=2.5f;z=0;fly(1,0,.9f,15);assert(x>.9f&&std::abs(y-2.5f)<.01f); // Strafe stays horizontal.
 j.remove();v.clear();j.appendExhaust(v,{0,3,0},0);assert(v.empty());assert(!j.thrustAllowed(false,false,false,false));
 for(int i=0;i<360;i++)p.update(scene,0,0,0,1.f/120,false,x,y,z);
 assert(y>1.68f&&y<1.72f); // Removal restores gravity.
 sandbox::Object car{8,0,0,0};assert(std::abs(sandbox::driverSeat(car).y-1.50f)<.001f);
 std::cout<<"Jetpack: look-directed flight, hover, walls/floor/ceiling, gravity, air animation and raised car seat OK\n";
}
