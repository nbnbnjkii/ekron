#pragma once
#include "BodyContacts.h"
#include "RigidContactMath.h"
namespace sandbox {
struct TriangleContact {physics::Vec3 normal{},point{};float depth=0;};
struct CityBodyFrame {
 physics::Vec3 p[8],centre{},axes[3];
 explicit CityBodyFrame(const Object&o){int i=0;
  for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f}){p[i]=bodyContactPoint(o,x,y,z);centre=centre+p[i++];}
  centre=centre*.125f;axes[0]=physics::normalized(p[4]-p[0]);axes[1]=physics::normalized(p[2]-p[0]);axes[2]=physics::normalized(p[1]-p[0]);
 }
};
inline bool frameTriangleContact(const CityBodyFrame&frame,const physics::Triangle&t,TriangleContact&contact,float skin=0){
 const auto&p=frame.p;const auto&axes=frame.axes;auto centre=frame.centre;
 physics::Vec3 edges[3]={t.b-t.a,t.c-t.b,t.a-t.c};auto n=physics::normalized(physics::cross(edges[0],edges[1]));
 auto overlaps=[&](physics::Vec3 axis){if(physics::length(axis)<1e-6f)return true;axis=physics::normalized(axis);float low=1e9f,high=-1e9f;
  for(auto point:p){float v=physics::dot(point,axis);low=std::min(low,v);high=std::max(high,v);}
  float a=physics::dot(t.a,axis),b=physics::dot(t.b,axis),c=physics::dot(t.c,axis);return high+skin>=std::min({a,b,c})&&low-skin<=std::max({a,b,c});
 };
 if(!overlaps(n))return false;
 for(auto axis:axes)if(!overlaps(axis))return false;
 for(auto axis:axes)for(auto edge:edges)if(!overlaps(physics::cross(axis,edge)))return false;
 float distance=physics::dot(centre-t.a,n);if(distance<0)n=n*-1;
 float bottom=1e9f;for(auto point:p)bottom=std::min(bottom,physics::dot(point-t.a,n));
 physics::Vec3 support{};int count=0;
 for(auto point:p)if(physics::dot(point-t.a,n)<bottom+.01f){support=support+point;count++;}
 contact={n,physics::closestTriangle(support*(1.f/count),t),-bottom};return skin>0?contact.depth>=-skin:contact.depth>.003f;
}
inline bool bodyTriangleContact(const Object&o,const physics::Triangle&t,TriangleContact&c,float skin=0){return frameTriangleContact(CityBodyFrame(o),t,c,skin);}
inline float recoverCityContact(Object&o,const physics::CollisionMesh&mesh){
 float total=0;
 for(int iteration=0;iteration<8&&total<.24f;iteration++){
  physics::Bounds area;for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})area.add(bodyContactPoint(o,x,y,z));
  CityBodyFrame frame(o);TriangleContact deepest;
  mesh.query(area,[&](const physics::Triangle&t){TriangleContact candidate;if(frameTriangleContact(frame,t,candidate)&&candidate.depth>deepest.depth)deepest=candidate;});
  if(deepest.depth<=.003f)break;
  float correction=std::min({deepest.depth+.001f,.08f,.24f-total});
  auto delta=deepest.normal*correction;
  if(deepest.normal.y>.64f)delta={0,correction/deepest.normal.y,0};
  o.x+=delta.x;o.y+=delta.y;o.z+=delta.z;total+=physics::length(delta);
  resolveStaticImpact(o,deepest.point,deepest.normal,0);
 }
 return total;
}
inline bool solveCityContactVelocities(Object&o,const physics::CollisionMesh&mesh,std::vector<physics::Vec3>*support=nullptr){
 physics::Bounds area;for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})area.add(bodyContactPoint(o,x,y,z));
 area.lo=area.lo-physics::Vec3{.008f,.008f,.008f};area.hi=area.hi+physics::Vec3{.008f,.008f,.008f};
 CityBodyFrame frame(o);std::vector<TriangleContact>contacts;contacts.reserve(32);bool wall=false;
 mesh.query(area,[&](const physics::Triangle&t){
  TriangleContact contact;if(!frameTriangleContact(frame,t,contact,.008f))return;
  if(std::abs(contact.normal.y)<.64f){
   // Contact skin must not turn the vertical side of a rock *below* the
   // chassis into a wall once the car has already climbed onto its top.
   float lowest=frame.p[0].y;for(auto p:frame.p)lowest=std::min(lowest,p.y);
   if(std::max({t.a.y,t.b.y,t.c.y})<lowest-.001f)return;
   wall=true;
  }
  // Clip the triangle against all six box planes. A single projected face
  // centre is not a contact manifold: near a pit seam it can lie outside the
  // actual overlap and inject a fresh rocking impulse every step.
  auto origin=frame.p[0];
  physics::Vec3 edges[]={frame.p[4]-origin,frame.p[2]-origin,frame.p[1]-origin};
  std::array<physics::Vec3,16> polygon{},next{};size_t count=3;polygon[0]=t.a;polygon[1]=t.b;polygon[2]=t.c;
  for(auto edge:edges){float extent=physics::length(edge);auto axis=edge*(1/extent);
   for(int side:{-1,1}){size_t size=0;
    auto distance=[&](physics::Vec3 p){float value=physics::dot(p-origin,axis);return side<0?-value-.008f:value-extent-.008f;};
    for(size_t i=0;i<count;i++){auto a=polygon[i],b=polygon[(i+1)%count];float da=distance(a),db=distance(b);
     if(da<=0)next[size++]=a;
     if((da<0)!=(db<0))next[size++]=a+(b-a)*(da/(da-db));
    }polygon=next;count=size;
   }
  }
  for(size_t i=0;i<count;i++){auto point=polygon[i];bool duplicate=false;
   for(const auto&c:contacts)if(physics::dot(c.normal,contact.normal)>.9999f&&physics::length(c.point-point)<.003f){duplicate=true;break;}
   if(!duplicate){auto c=contact;c.point=point;contacts.push_back(c);}
  }
 });
 if(support)for(const auto&c:contacts)if(c.normal.y>.64f)support->push_back(c.point);
 auto bounds=World::bounds(o);ContactInertia inertia(o);auto material=contactMaterial(o);
 struct Constraint {physics::Vec3 normal,leverX,leverZ,friction{};float normalMass=1,impulse=0;};
 std::vector<Constraint> constraints;constraints.reserve(contacts.size());
 auto denominator=[&](const Constraint&c,physics::Vec3 direction){float x=physics::dot(c.leverX,direction),z=physics::dot(c.leverZ,direction);return 1+x*x/inertia.ix+z*z/inertia.iz;};
 for(const auto&contact:contacts){auto r=contact.point-physics::Vec3{bounds.x,bounds.y,bounds.z};Constraint c;c.normal=contact.normal;c.leverX=physics::cross(inertia.axisX,r);c.leverZ=physics::cross({0,0,1},r);c.normalMass=1/denominator(c,c.normal);constraints.push_back(c);}
 auto velocity=[&](const Constraint&c){return physics::Vec3{o.vx,o.vy,o.vz}+c.leverX*o.spinX+c.leverZ*o.spinZ;};
 auto apply=[&](const Constraint&c,physics::Vec3 impulse){o.vx+=impulse.x;o.vy+=impulse.y;o.vz+=impulse.z;o.spinX+=physics::dot(c.leverX,impulse)/inertia.ix;o.spinZ+=physics::dot(c.leverZ,impulse)/inertia.iz;};
 for(int pass=0;pass<48;pass++){float largestChange=0;for(size_t k=0;k<constraints.size();k++){
  auto&c=constraints[pass%2?constraints.size()-1-k:k];
  float incoming=physics::dot(velocity(c),c.normal);
  float nextNormal=std::max(0.f,c.impulse-incoming*c.normalMass);
  largestChange=std::max(largestChange,std::abs(nextNormal-c.impulse));
  apply(c,c.normal*(nextNormal-c.impulse));c.impulse=nextNormal;
  auto v=velocity(c);auto tangent=v-c.normal*physics::dot(v,c.normal);float speed=physics::length(tangent);
  auto candidate=c.friction;if(speed>1e-7f){auto direction=tangent*(1/speed);candidate=candidate-direction*(speed/denominator(c,direction));}
  float size=physics::length(candidate),limit=material.staticFriction*nextNormal;
  if(size>limit&&size>1e-9f)candidate=candidate*(limit/size);
  largestChange=std::max(largestChange,physics::length(candidate-c.friction));
  apply(c,candidate-c.friction);c.friction=candidate;
 }
 if(largestChange<1e-4f)break;
 }
 return wall;
}

}
