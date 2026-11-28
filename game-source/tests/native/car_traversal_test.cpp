#include "../../sys/android/app/src/main/cpp/CarTraversal.h"
#include "../../sys/android/app/src/main/cpp/ObjectOverlap.h"
#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include <cassert>
#include <iostream>
using namespace sandbox;
bool attempt(float height,bool ceiling=false,bool pit=false){
 physics::CollisionMesh mesh;
 if(!pit)mesh.addBox({-20,-1,-20},{20,0,20});
 mesh.addBox({2.04f,0,-4},{4,height,4});
 if(ceiling)mesh.addBox({-10,2.1f,-10},{10,3,10});
 mesh.build();Object car{8,0,0,0};car.y=.09f;
 auto ground=[&](float x,float z,float top,float bottom){return mesh.groundHeight(x,z,top,bottom);};
 auto blocked=[&](float x,float y,float z){auto b=World::bounds(car);Object at=car;at.x=x;at.y=y;at.z=z;return objectOverlap(mesh,at,{x,y+b.h/2,z});};
 bool result=tryCarTerrainStep(car,.12f,0,ground,blocked);
 if(result){assert(car.y<=.54f);assert(!blocked(.12f,car.y,0));}
 return result;
}
void driveOver(float height){
 physics::CollisionMesh mesh;mesh.addBox({-20,-1,-20},{20,0,20});mesh.addBox({2.1f,0,-4},{4,height,4});mesh.build();
 World w;w.objects.clear();Object car{8,0,-1,0};car.y=.1f;w.objects.push_back(car);ObjectDynamics dynamics;
 for(int i=0;i<600;i++){auto&c=w.objects[0];c.vx=4;c.vz=0;c.awake=true;dynamics.update(w,mesh,1.f/120,-1,{-20,-5,-20},{20,10,20});}
 assert(w.objects[0].x>5);
}
int main(){
 driveOver(.20f);driveOver(.35f);
 assert(attempt(.20f));assert(attempt(.35f));assert(attempt(.44f));
 assert(!attempt(1.2f));assert(!attempt(.35f,true));assert(!attempt(.35f,false,true));
 std::cout<<"Car traversal: 20/35/44 cm obstacles pass; walls, low ceiling and unsupported air blocked OK\n";
}
