#pragma once
#include "ObjectMesh.h"
#include "PlayerPhysics.h"
#include <optional>
namespace sandbox {
inline physics::Vec3 doorPoint(const Object&o,physics::Vec3 p){
 float hx=-.55f,hz=0;for(int i=0;i<o.rotation;i++){float old=hx;hx=hz;hz=-old;}
 float x=p.x-o.x-hx,z=p.z-o.z-hz,c=std::cos(o.doorAngle),s=std::sin(o.doorAngle);
 return {o.x+hx+c*x+s*z,p.y,o.z+hz-s*x+c*z};
}
inline void animateDoorMesh(const Object&o,std::vector<MeshVertex>&v,size_t begin){
 if(o.type!=3)return;
 float c=std::cos(o.doorAngle),s=std::sin(o.doorAngle);
 for(size_t i=begin;i<v.size();i++){auto&p=v[i];auto q=doorPoint(o,{p.x,p.y,p.z});p.x=q.x;p.z=q.z;float nx=p.nx;p.nx=c*nx+s*p.nz;p.nz=-s*nx+c*p.nz;}
}
inline void addObjectCollision(physics::CollisionMesh&mesh,const Object&o){
 if(o.type==3){std::vector<MeshVertex> v;appendObject(v,o);animateDoorMesh(o,v,0);
  auto b=World::bounds(o);
  for(auto&p:v){float x=p.x-b.x,y=p.y-b.y,z=p.z-b.z;float py=std::cos(o.tiltX)*y-std::sin(o.tiltX)*z,pz=std::sin(o.tiltX)*y+std::cos(o.tiltX)*z;p.x=b.x+std::cos(o.tiltZ)*x-std::sin(o.tiltZ)*py;p.y=b.y+std::sin(o.tiltZ)*x+std::cos(o.tiltZ)*py;p.z=b.z+pz;}
  for(size_t i=0;i<v.size();i+=3)mesh.add({{v[i].x,v[i].y,v[i].z},{v[i+1].x,v[i+1].y,v[i+1].z},{v[i+2].x,v[i+2].y,v[i+2].z}});}
 else {auto b=World::bounds(o);
  if(std::abs(o.tiltX)+std::abs(o.tiltZ)<.001f&&std::abs(o.yaw)<.001f){mesh.addBox({b.x-b.w/2,b.y-b.h/2,b.z-b.d/2},{b.x+b.w/2,b.y+b.h/2,b.z+b.d/2});return;}
  std::vector<MeshVertex> vertices;appendCube(vertices,b);
  auto point=[&](const MeshVertex&v){float x=v.x-b.x,y=v.y-b.y,z=v.z-b.z;float cy=std::cos(o.yaw),sy=std::sin(o.yaw);float wx=cy*x+sy*z,wz=-sy*x+cy*z;x=wx;z=wz;float py=std::cos(o.tiltX)*y-std::sin(o.tiltX)*z,pz=std::sin(o.tiltX)*y+std::cos(o.tiltX)*z;return physics::Vec3{b.x+std::cos(o.tiltZ)*x-std::sin(o.tiltZ)*py,b.y+std::sin(o.tiltZ)*x+std::cos(o.tiltZ)*py,b.z+pz};};
  for(size_t i=0;i<vertices.size();i+=3)mesh.add({point(vertices[i]),point(vertices[i+1]),point(vertices[i+2])});
 }
}
inline int interactionTarget(const World&w,const physics::CollisionMesh&city){
 physics::Vec3 origin{w.x,w.y,w.z},dir{-std::sin(w.yaw)*std::cos(w.pitch),std::sin(w.pitch),-std::cos(w.yaw)*std::cos(w.pitch)};
 physics::CollisionMesh::RayHit hit;float distance=3;int target=-1;if(city.raycast(origin,dir,distance,hit))distance=hit.distance;
 for(size_t i=0;i<w.objects.size();i++){physics::CollisionMesh mesh;addObjectCollision(mesh,w.objects[i]);mesh.build();if(mesh.raycast(origin,dir,distance,hit)){distance=hit.distance;target=(w.objects[i].type==3||w.objects[i].type==8||w.objects[i].type==7)?int(i):-1;}}
 return target;
}
inline physics::Vec3 driverSeat(const Object&o){float x=o.type==7?0:.55f,z=o.type==7?.08f:-.45f;float c=std::cos(o.yaw),s=std::sin(o.yaw);return {o.x+c*x+s*z,o.y+(o.type==7?1.48f:1.35f),o.z-s*x+c*z};}
inline std::optional<physics::Vec3> carExit(const World&w,int index,const physics::CollisionMesh&city,const physics::CollisionMesh&built,const std::array<float,3>&lo,const std::array<float,3>&hi){
 auto b=World::bounds(w.objects[index]);physics::PlayerController player;
 for(float extra:{.65f,1.2f})for(auto offset:{physics::Vec3{b.w/2+extra,0,0},physics::Vec3{-b.w/2-extra,0,0},physics::Vec3{0,0,b.d/2+extra},physics::Vec3{0,0,-b.d/2-extra}}){
  float x=b.x+offset.x,z=b.z+offset.z;if(x<lo[0]+.4f||x>hi[0]-.4f||z<lo[2]+.4f||z>hi[2]-.4f)continue;
  float floor=std::max(city.groundHeight(x,z,b.y+2,lo[1]-1),built.groundHeight(x,z,b.y+2,lo[1]-1));float y=floor+1.701f;
  if(std::isfinite(y)&&!player.overlaps(city,x,y,z)&&!player.overlaps(built,x,y,z))return physics::Vec3{x,y,z};
 }return {};
}
}
