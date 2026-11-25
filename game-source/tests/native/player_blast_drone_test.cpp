#include "../../sys/android/app/src/main/cpp/PlayerView.h"
#include "../../sys/android/app/src/main/cpp/BlastEffects.h"
#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include "../../sys/android/app/src/main/cpp/FpvDrone.h"
#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace physics;using namespace sandbox;
 CollisionMesh empty,floor,wall;empty.build();floor.addBox({-20,-.2f,-20},{20,0,20});floor.build();wall.addBox({-5,0,1},{5,4,1.2f});wall.build();
 PlayerView view;Vec3 eye{0,1.7f,0};view.toggle();auto first=view.camera(eye,0,0,.016f,true,empty,empty);assert(length(first-eye)<.02f);
 for(int i=0;i<60;i++)view.camera(eye,0,0,.016f,true,empty,empty);
 auto full=view.camera(eye,0,0,0,true,empty,empty);assert(full.z>3.4f&&full.z<3.6f);
 auto clipped=view.camera(eye,0,0,0,true,wall,empty);assert(clipped.z<.85f&&!wall.intersectsBox(clipped,{.16f,.16f,.16f}));
 auto builtClipped=view.camera(eye,0,0,0,true,empty,wall);assert(builtClipped.z<.85f);
 view.toggle();for(int i=0;i<60;i++)view.camera(eye,0,0,.016f,true,empty,empty);assert(length(view.camera(eye,0,0,0,true,empty,empty)-eye)<.001f);
 std::vector<MeshVertex> avatar;view.append(avatar,eye,false,false);assert(avatar.size()>1000);
 World w;w.objects={{30,0,1,0},{32,0,2,0}};BlastEffects effects;effects.explode(w,{0,.5f,0});
 assert(effects.fragmentCount()==36);assert(w.objects[0].vx>0&&w.objects[0].vy>0);
 for(int i=0;i<100;i++)effects.explode(w,{0,.5f,0});
 assert(effects.fragmentCount()==128);
 for(int i=0;i<100;i++)effects.update(.05f,floor,empty);
 assert(effects.fragmentCount()==0);
 FpvDrone drone;drone.mode=FpvDrone::Flying;drone.position={0,1,0};drone.velocity={0,0,15};drone.update(.05f,0,0,0,0,0,wall,empty);assert(drone.crashPending&&drone.mode==FpvDrone::Absent&&!drone.linked);
 FpvDrone gentle;gentle.mode=FpvDrone::Flying;gentle.position={0,1,.6f};gentle.velocity={0,0,.1f};gentle.update(.05f,0,0,0,0,0,wall,empty);assert(!gentle.crashPending);
 for(int type=29;type<kObjectTypeCount;type++){
  World props;props.objects={{type,0,0,-3}};props.x=0;props.z=0;props.y=World::bounds(props.objects[0]).h/2;
  ToolInteraction tool;tool.tool=HandTool::GRAVITY_GUN;assert(tool.grab(props,nullptr));assert(tool.release(props));assert(props.objects[0].vz< -1);
  props.objects[0].vx=props.objects[0].vy=props.objects[0].vz=0;props.objects[0].y=.15f;ObjectDynamics falling;
  for(int i=0;i<60;i++)falling.update(props,floor,.016f,-1,{-20,-2,-20},{20,20,20});
  assert(props.objects.size()==1&&props.objects[0].y<.15f);
 }
 World barrels;barrels.objects.clear();barrels.objects.push_back({29,0,0,0});barrels.objects[0].y=2;barrels.objects[0].vy=-10;ObjectDynamics dynamics;bool exploded=false;
 for(int i=0;i<60&&!exploded;i++){dynamics.update(barrels,floor,.016f,-1,{-20,-2,-20},{20,20,20});exploded=!dynamics.barrelImpacts.empty();}
 assert(exploded&&barrels.objects.empty());
 World resting;resting.objects.clear();resting.objects.push_back({29,0,0,0});ObjectDynamics slow;
 for(int i=0;i<60;i++){slow.update(resting,floor,.016f,-1,{-20,-2,-20},{20,20,20});assert(slow.barrelImpacts.empty());}
 std::cout<<"Player body/first-third view/swept camera, bounded blast debris, barrel impact, drone hard crash and gentle contact OK\n";
}
