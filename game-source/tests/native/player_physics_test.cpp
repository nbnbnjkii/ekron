#include "../../sys/android/app/src/main/cpp/PlayerPhysics.h"
#include <cassert>
#include <iostream>
using namespace physics;
void floorQuad(CollisionMesh&m,float y,float x0=-20,float x1=20,float z0=-20,float z1=20){m.add({{x0,y,z0},{x1,y,z1},{x1,y,z0}});m.add({{x0,y,z0},{x0,y,z1},{x1,y,z1}});}
int main(){
 CollisionMesh scene;floorQuad(scene,0);floorQuad(scene,5,-3,3,-3,3);scene.build();
 PlayerController p;float x=0,y=9,z=0;p.reset(x,y,z);
 float old=y;p.update(scene,0,0,0,.016f,false,x,y,z);assert(x==0&&z==0&&y<old&&y>8.9f); // No landing teleport.
 for(int i=0;i<300;i++)p.update(scene,0,0,0,1.f/60,false,x,y,z);
 assert(std::abs(y-6.7f)<.02f&&p.grounded()); // Remain on roof, not ground.
 for(int i=0;i<130;i++)p.update(scene,1,0,0,1.f/60,false,x,y,z);
 for(int i=0;i<200;i++)p.update(scene,0,0,0,1.f/60,false,x,y,z);
 assert(std::abs(y-1.7f)<.02f); // Walk off roof and fall.
 p.jump();for(int i=0;i<15;i++)p.update(scene,0,0,0,1.f/60,false,x,y,z);assert(y>2);
 CollisionMesh pit;floorQuad(pit,-2);floorQuad(pit,0,-20,-2);floorQuad(pit,0,2,20);pit.build();
 x=0;y=1.7f;z=0;p.reset(x,y,z);for(int i=0;i<300;i++)p.update(pit,0,0,0,1.f/60,false,x,y,z);assert(std::abs(y+.3f)<.02f); // Real pit bottom.
 CollisionMesh slope;slope.add({{-10,-2,-10},{10,2,10},{10,2,-10}});slope.add({{-10,-2,-10},{-10,-2,10},{10,2,10}});slope.build();
 x=-5;y=3;z=0;p.reset(x,y,z);for(int i=0;i<300;i++)p.update(slope,0,0,0,1.f/60,false,x,y,z);
 float stable=y;for(int i=0;i<120;i++)p.update(slope,0,0,0,1.f/60,false,x,y,z);assert(std::abs(y-stable)<.005f);
 CollisionMesh window;floorQuad(window,0);window.addBox({-3,0,-.1f},{-1,3,.1f});window.addBox({1,0,-.1f},{3,3,.1f});window.addBox({-1,0,-.1f},{1,.65f,.1f});window.addBox({-1,2,-.1f},{1,3,.1f});window.build();
 // Short capsule fits the actual opening; standing capsule cannot fit through its lintel.
 // Hold a crouched capsule above the solid sill while crossing the empty opening.
 PlayerController opening;x=0;y=2.4f;z=.36f;opening.reset(x,y,z);
 for(int i=0;i<65;i++)opening.update(window,0,1,0,1.f/120,true,x,y,z);
 assert(z<-.3f);
 x=2;y=1.7f;z=1;p.reset(x,y,z);for(int i=0;i<100;i++)p.update(window,0,1,0,1.f/120,false,x,y,z);assert(z>.3f);
 // Prone is real capsule geometry, not just a lower camera. Cannot stand in a low tunnel.
 CollisionMesh tunnel;floorQuad(tunnel,0);tunnel.addBox({-3,.7f,-3},{3,1.1f,3});tunnel.build();
 PlayerController crawling;x=0;y=1.7f;z=0;crawling.reset(x,y,z);
 for(int i=0;i<120;i++)crawling.update(tunnel,0,0,0,1.f/120,false,x,y,z,nullptr,true);
 assert(std::abs(crawling.eyeHeight()-.4f)<.001f&&std::abs(y-.4f)<.02f);
 float before=z;
 for(int i=0;i<60;i++)crawling.update(tunnel,0,1,0,1.f/120,false,x,y,z,nullptr,true);
 assert(before-z>.45f&&before-z<.55f);
 for(int i=0;i<30;i++)crawling.update(tunnel,0,0,0,1.f/120,false,x,y,z);
 assert(crawling.eyeHeight()<.5f); // Refuse standing up through the ceiling.
 // Keep walking input while a second finger requests jump: horizontal travel must continue.
 PlayerController running;x=5;y=1.7f;z=5;running.reset(x,y,z);
 for(int i=0;i<60;i++)running.update(scene,0,0,0,1.f/120,false,x,y,z);
 running.jump();float startZ=z;float peak=y;
 for(int i=0;i<50;i++){running.update(scene,0,1,0,1.f/120,false,x,y,z);peak=std::max(peak,y);}
 assert(startZ-z>1.0f&&peak>2.3f);
 // A 65 cm windowsill is crossed without any jump command; the camera rises smoothly.
 CollisionMesh sill;floorQuad(sill,0);sill.addBox({-2,0,-.2f},{2,.65f,.2f});sill.build();
 PlayerController walking;x=0;y=1.7f;z=1;walking.reset(x,y,z);
 for(int i=0;i<60;i++)walking.update(sill,0,0,0,1.f/120,false,x,y,z);
 float maxFeet=0,previousY=y,maxCameraDelta=0;
 for(int i=0;i<140;i++){
  walking.update(sill,0,1,0,1.f/120,false,x,y,z);maxFeet=std::max(maxFeet,walking.feet().y);
  maxCameraDelta=std::max(maxCameraDelta,std::abs(y-previousY));previousY=y;
 }
 assert(z<-1&&maxFeet>.64f&&maxFeet<.8f&&maxCameraDelta<.15f);
 // Taller obstacles and a low concrete lintel must NOT be bypassed by step-up.
 for(bool lintel:{false,true}){
  CollisionMesh obstacle;floorQuad(obstacle,0);
  obstacle.addBox({-2,0,-.2f},{2,lintel?.65f:1.1f,.2f});
  if(lintel)obstacle.addBox({-2,2,-.2f},{2,3,.2f});
  obstacle.build();PlayerController blocked;x=0;y=1.7f;z=1;blocked.reset(x,y,z);
  for(int i=0;i<100;i++)blocked.update(obstacle,0,1,0,1.f/120,false,x,y,z);
  assert(z>.4f&&blocked.feet().y<.1f);
 }
 // Full stick is WALK; sprint is explicit, and never accelerates crouch/prone.
 CollisionMesh track;floorQuad(track,0);track.build();
 auto travel=[&](bool sprint,bool crouch,bool prone,float right,float forward){
  PlayerController p;float px=0,py=1.7f,pz=0;p.reset(px,py,pz);
  for(int i=0;i<120;i++)p.update(track,right,forward,0,1.f/120,crouch,px,py,pz,nullptr,prone,nullptr,sprint);
  return std::hypot(px,pz);
 };
 float walk=travel(false,false,false,0,1),run=travel(true,false,false,0,1);
 assert(walk>2.6f&&walk<2.8f&&run>5.7f&&run<5.9f);
 assert(std::abs(travel(true,true,false,0,1)-travel(false,true,false,0,1))<.01f);
 assert(std::abs(travel(true,false,true,0,1)-travel(false,false,true,0,1))<.01f);
 assert(std::abs(travel(false,false,false,1,1)-walk)<.01f);
 std::cout<<"Gravity, roof landing, pits, stable slopes, jump/crouch window and concrete frame OK\n";
}
