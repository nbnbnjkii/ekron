#include "../../sys/android/app/src/main/cpp/ObjectMesh.h"
#include "../../sys/android/app/src/main/cpp/Interaction.h"
#include "../../sys/android/app/src/main/cpp/BodyContacts.h"
#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 using namespace sandbox;using namespace physics;
 const float hpi=1.57079632679f;
 // Yaw 0 keeps the old axis-aligned behaviour.
 Object flat{8,0,0,0};flat.yaw=0.f;
 auto front=objectCorner(flat,1,0,0);assert(std::abs(front.x-4.025f/2)<.001f&&std::abs(front.z)<.001f);
 // Yaw +90deg rotates straight ahead (+X) onto the -Z axis, matching drawCar.
 Object turned{8,0,0,0};turned.yaw=hpi;
 auto nose=objectCorner(turned,1,0,0);assert(std::abs(nose.x)<.001f&&std::abs(nose.z+4.025f/2)<.001f);
 // driverSeat agrees with the former discrete rotation=1 offset (-.45,-.55).
 auto seat=driverSeat(turned);assert(std::abs(seat.x-(-.45f))<.001f&&std::abs(seat.z-(-.55f))<.001f);
 // Collision is the oriented box, not the unrotated AABB.
 CollisionMesh mesh;addObjectCollision(mesh,turned);mesh.build();assert(mesh.size()==12);
 CollisionMesh::RayHit hit;
 assert(mesh.raycast({0,0.96f,-3},{0,0,1},6,hit)); // front face now faces -Z
 assert(hit.distance<1.1f);
 assert(!mesh.raycast({3,0.96f,0},{1,0,0},6,hit));  // old +X front is empty
 // Yaw 0 still yields the fast AABB path (no per-frame oriented rebuild).
 CollisionMesh aabb;addObjectCollision(aabb,flat);aabb.build();assert(aabb.size()==12);
 // Discrete heading rounding must not swap an already continuously rotated chassis.
 turned.rotation=1;auto roundedNose=objectCorner(turned,1,0,0);
 assert(std::abs(roundedNose.z+4.025f/2)<.001f);
 // A side wall 15 cm outside the real chassis must not block it at 90deg.
 CollisionMesh side;side.addBox({1.1f,-1,-5},{1.2f,4,5});side.build();
 auto bounds=World::bounds(turned);
 assert(!objectOverlap(side,turned,{bounds.x,bounds.y,bounds.z}));
 CollisionMesh touching;touching.addBox({.8f,-1,-5},{.9f,4,5});touching.build();
 assert(objectOverlap(touching,turned,{bounds.x,bounds.y,bounds.z}));
 World citySave;citySave.x=120;citySave.y=3;citySave.z=104;citySave.yaw=.63f;citySave.equippedJetpack=2;citySave.objects={{8,0,24,104},{12,0,500,-650}};
 assert(citySave.save("/tmp/ekron-large-map-save.txt"));World restored;assert(restored.load("/tmp/ekron-large-map-save.txt"));
 assert(restored.objects.size()==2&&restored.objects[0].z==104);assert(restored.hasPlayerState&&restored.x==120&&restored.y==3&&restored.equippedJetpack==2&&std::abs(restored.yaw-.63f)<.001f);std::remove("/tmp/ekron-large-map-save.txt");
 // Powered traversal clears a low kerb, but does not cross a tall wall.
 for(float obstacleHeight:{.12f,.20f,2.f}){
  CollisionMesh track;track.addBox({-30,-1,-30},{30,0,30});track.addBox({2.5f,0,-3},{3.f,obstacleHeight,3});track.build();
  World driving;driving.objects={{8,0,0,0}};ObjectDynamics dynamics;
  for(int i=0;i<120;i++)dynamics.update(driving,track,1.f/120,-1,{-30,-5,-30},{30,20,30});
  for(int i=0;i<480;i++){driving.objects[0].vx=3;driving.objects[0].awake=true;dynamics.update(driving,track,1.f/120,-1,{-30,-5,-30},{30,20,30});}
  if(obstacleHeight<.3f)assert(driving.objects[0].x>5.f);else assert(driving.objects[0].x<.6f);
 }
 std::cout<<"Car yaw: corners, seat and oriented collision consistent OK\n";
 return 0;
}