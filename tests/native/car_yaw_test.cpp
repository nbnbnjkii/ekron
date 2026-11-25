#include "../../sys/android/app/src/main/cpp/ObjectMesh.h"
#include "../../sys/android/app/src/main/cpp/Interaction.h"
#include "../../sys/android/app/src/main/cpp/BodyContacts.h"
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
 std::cout<<"Car yaw: corners, seat and oriented collision consistent OK\n";
 return 0;
}