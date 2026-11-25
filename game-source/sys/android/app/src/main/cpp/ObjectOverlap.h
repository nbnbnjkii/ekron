#pragma once
#include "ObjectCcd.h"
namespace sandbox {
inline bool objectOverlap(const physics::CollisionMesh&mesh,const Object&o,physics::Vec3 centre,float scale=1.f){
 auto b=World::bounds(o);physics::Vec3 base{b.x,b.y,b.z},shift=centre-base;
 physics::Bounds area;for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})area.add(objectCorner(o,x,y,z)+shift);
 auto local=[&](physics::Vec3 p){p=p-centre;float x=std::cos(o.tiltZ)*p.x+std::sin(o.tiltZ)*p.y,y=-std::sin(o.tiltZ)*p.x+std::cos(o.tiltZ)*p.y;float yy=std::cos(o.tiltX)*y+std::sin(o.tiltX)*p.z,zz=-std::sin(o.tiltX)*y+std::cos(o.tiltX)*p.z;return physics::Vec3{std::cos(o.yaw)*x-std::sin(o.yaw)*zz,yy,std::sin(o.yaw)*x+std::cos(o.yaw)*zz};};
 physics::CollisionMesh transformed;mesh.query(area,[&](const physics::Triangle&t){transformed.add({local(t.a),local(t.b),local(t.c)});});transformed.build();
 return transformed.intersectsBox({0,0,0},{std::max(.005f,b.w*scale/2-.008f),std::max(.005f,b.h*scale/2-.008f),std::max(.005f,b.d*scale/2-.008f)});
}
}
