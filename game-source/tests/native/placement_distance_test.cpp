#include "../../sys/android/app/src/main/cpp/RaycastPlacement.h"
#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include <cassert>
#include <iostream>
int main(){using namespace sandbox;using namespace physics;
 World w;w.objects.clear();w.selected=12;w.x=0;w.y=2;w.z=0;w.yaw=w.pitch=0;
 CollisionMesh empty;empty.build();std::array<float,3> lo{-30,-5,-30},hi{30,30,30};
 auto near=RaycastPlacement::preview(w,empty,empty,lo,hi,3);
 auto far=RaycastPlacement::preview(w,empty,empty,lo,hi,6);
 assert(near&&far&&std::abs(near->z+3)<.001f&&std::abs(far->z+6)<.001f);
 assert(World::bounds(*near).w==World::bounds(*far).w&&!far->anchored);
 auto committed=RaycastPlacement::commitPreview(w,far,empty,empty,lo,hi);assert(committed&&committed->z==far->z);
 CollisionMesh wall;wall.addBox({-10,-2,-5},{10,10,-4});wall.build();
 auto blocked=RaycastPlacement::preview(w,wall,empty,lo,hi,10);assert(blocked&&blocked->z>-4);
 auto capped=RaycastPlacement::preview(w,empty,empty,lo,hi,100);assert(capped&&std::abs(capped->z+12)<.001f);
 ToolInteraction tool;tool.tool=HandTool::GRAVITY_GUN;w.objects={{12,0,0,-3}};w.objects[0].y=1.5f;assert(tool.grab(w,&empty));
 tool.adjustDistance(2);assert(tool.holdDistance==5.5f);tool.pull(w,.016f,&wall);assert(tool.display.z>-4);
 tool.adjustDistance(100);assert(tool.holdDistance==12);tool.adjustDistance(-100);assert(tool.holdDistance==1.5f);
 std::cout<<"Placement/held distance, constant size, preview commit and wall blocking OK\n";
}
