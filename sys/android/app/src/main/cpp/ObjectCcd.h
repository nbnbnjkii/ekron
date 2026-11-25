#pragma once
#include "SandboxWorld.h"
#include "CollisionMesh.h"
#include "RigidContactMath.h"
namespace sandbox {
inline physics::Vec3 objectCorner(const Object&o,float x,float y,float z){
 auto b=World::bounds(o);x*=b.w/2;y*=b.h/2;z*=b.d/2;float cy=std::cos(o.yaw),sy=std::sin(o.yaw);float wx=cy*x+sy*z,wz=-sy*x+cy*z;x=wx;z=wz;
 float py=std::cos(o.tiltX)*y-std::sin(o.tiltX)*z,pz=std::sin(o.tiltX)*y+std::cos(o.tiltX)*z;
 return {o.x+std::cos(o.tiltZ)*x-std::sin(o.tiltZ)*py,b.y+std::sin(o.tiltZ)*x+std::cos(o.tiltZ)*py,o.z+pz};
}
// Sweep actual rotated corners, not just upright XZ support probes. Angular
// subdivision bounds the curved corner path; floor hits stop at time of impact.
inline bool sweepObjectFloor(const Object&before,Object&after,const physics::CollisionMesh&mesh,bool includeWalls=false){
 int steps=std::clamp(int(std::ceil((std::abs(after.tiltX-before.tiltX)+std::abs(after.tiltZ-before.tiltZ))/.025f)),1,32);
 auto at=[&](float t){Object o=before;o.x+=(after.x-before.x)*t;o.y+=(after.y-before.y)*t;o.z+=(after.z-before.z)*t;o.tiltX+=(after.tiltX-before.tiltX)*t;o.tiltZ+=(after.tiltZ-before.tiltZ)*t;return o;};
 float earliest=2;physics::Vec3 contactNormal{},contactPoint{};
 for(int part=0;part<steps;part++){
  auto a=at(float(part)/steps),b=at(float(part+1)/steps);
  for(float x:{-1.f,0.f,1.f})for(float y:{-1.f,0.f,1.f})for(float z:{-1.f,0.f,1.f}){
   auto p=objectCorner(a,x,y,z),q=objectCorner(b,x,y,z),d=q-p;float length=physics::length(d);
   if(length<.00001f||(!includeWalls&&d.y>=0))continue;
   physics::CollisionMesh::RayHit hit;
   if(mesh.raycast(p,d,length+.0001f,hit)&&(includeWalls||hit.normal.y>.64f)){
    // A trailing face leaving a shallow overlap is not a new impact. Ray
    // normals are two-sided; without this check an item is pinned by the
    // underside above it while trying to fall away from that surface.
    auto ba=World::bounds(a),bb=World::bounds(b);float fraction=hit.distance/length;
    physics::Vec3 centre{ba.x+(bb.x-ba.x)*fraction,ba.y+(bb.y-ba.y)*fraction,ba.z+(bb.z-ba.z)*fraction};
    auto position=p+d*fraction;if(physics::dot(centre-position,hit.normal)<-.003f)continue;
    float t=(part+hit.distance/length)/steps;if(t<earliest){earliest=t;contactNormal=hit.normal;contactPoint=p+d*(hit.distance/length);}}
  }
 }
 if(earliest>1)return false;
 auto contact=at(std::max(0.f,earliest-.0001f));
 // Do not translate down a slope on every contact. Ground skin is vertical;
 // wall skin is along its normal. The impulse uses the real triangle normal.
 auto offset=contactNormal.y>.64f?physics::Vec3{0,.002f,0}:contactNormal*.002f;
 after.x=contact.x+offset.x;after.y=contact.y+offset.y;after.z=contact.z+offset.z;after.tiltX=contact.tiltX;after.tiltZ=contact.tiltZ;
 resolveStaticImpact(after,contactPoint,contactNormal,contactNormal.y>.64f?.05f:.35f);
 return true;
}
}
