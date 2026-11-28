#pragma once
#include "CollisionMesh.h"
#include "MeshGeometry.h"
namespace sandbox {
inline void ellipsoid(std::vector<MeshVertex>&out,physics::Vec3 p,physics::Vec3 size,physics::Vec3 color){
 constexpr int rings=8,sides=12;
 auto vertex=[&](int r,int s){float a=3.14159265f*r/rings,b=6.2831853f*s/sides;physics::Vec3 n{std::sin(a)*std::cos(b),std::cos(a),std::sin(a)*std::sin(b)};
 return MeshVertex{p.x+n.x*size.x,p.y+n.y*size.y,p.z+n.z*size.z,color.x,color.y,color.z,n.x,n.y,n.z,0,0,float(Plain)};};
 for(int r=0;r<rings;r++)for(int s=0;s<sides;s++)for(auto v:{vertex(r,s),vertex(r+1,s),vertex(r+1,s+1),vertex(r,s),vertex(r+1,s+1),vertex(r,s+1)})out.push_back(v);
}
struct PlayerView {
 bool third=false;float mix=0,bodyYaw=0,walk=0,speed=0,idleTime=0;
 physics::Vec3 previous{};bool initialized=false;
 void toggle(){third=!third;}
 physics::Vec3 camera(physics::Vec3 eye,float yaw,float pitch,float dt,bool enabled,const physics::CollisionMesh&city,const physics::CollisionMesh&built){
  mix=std::clamp(mix+(third?1.f:-1.f)*std::clamp(dt,0.f,.05f)/.55f,0.f,1.f);
  if(!enabled)return eye;
  float t=mix*mix*(3-2*mix);
  physics::Vec3 back{std::sin(yaw)*std::cos(pitch)*3.5f,-std::sin(pitch)*3.5f+.25f,std::cos(yaw)*std::cos(pitch)*3.5f};
  auto delta=back*t;int steps=std::max(1,int(std::ceil(physics::length(delta)/.035f)));auto safe=eye;
  for(int i=1;i<=steps;i++){auto p=eye+delta*(float(i)/steps);if(city.intersectsBox(p,{.16f,.16f,.16f})||built.intersectsBox(p,{.16f,.16f,.16f}))break;safe=p;}
  return safe;
 }
 void updateBody(physics::Vec3 eye,float yaw,float dt){
  if(!initialized){previous=eye;bodyYaw=yaw;initialized=true;}
  auto d=eye-previous;float distance=std::hypot(d.x,d.z);float target=distance>.001f&&distance<1?std::atan2(-d.x,-d.z):yaw;
  bodyYaw+=std::remainder(target-bodyYaw,6.2831853f)*std::min(1.f,dt*12);walk+=std::min(distance,.25f)*7;
  float instant=dt>0?distance/dt:0;speed+=(std::min(instant,6.f)-speed)*std::min(1.f,dt*8);
  idleTime+=dt;previous=eye;
 }
 void append(std::vector<MeshVertex>&out,physics::Vec3 eye,bool crouch,bool prone,float actualEyeHeight=0)const{
  size_t begin=out.size();float height=actualEyeHeight>0?actualEyeHeight:prone?.4f:crouch?.9f:1.7f;float floor=eye.y-height;
  // Fixed body proportions: crouching bends the knees (two-bone IK keeps the
  // feet on the floor) and leans the torso, prone lies the body down. The
  // player never shrinks. Local -Z is the facing direction; each box rotates
  // around its joint, then its root, then the hip (spine lean).
  float hipH=prone?.9f:std::clamp(height-.8f,.5f,.9f);
  float amp=std::clamp(speed/3.5f,0.f,1.f);
  float breath=std::sin(idleTime*1.9f)*.008f*(1.f-amp);
  float bob=(std::abs(std::sin(walk))-.5f)*.05f*amp;
  float lean=-amp*.09f-(.9f-hipH)*.9f;
  float hip=floor+hipH; // parts and pivots below add bob/breath once, uniformly
  auto box=[&](float cx,float cy,float cz,float hx,float hy,float hz,physics::Vec3 color,float jY,float jA,float rY,float rA,float sA){
   static constexpr int faces[6][6]={{0,1,2,0,2,3},{5,4,7,5,7,6},{4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
   static constexpr float corners[8][3]={{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1},{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1}};
   static constexpr float normals[6][3]={{0,0,1},{0,0,-1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0}};
   float cj=std::cos(jA),sj=std::sin(jA),cr=std::cos(rA),sr=std::sin(rA),cs=std::cos(sA),ss=std::sin(sA);
   for(int f=0;f<6;f++)for(int i:faces[f]){
    float x=corners[i][0]*hx,y=cy+corners[i][1]*hy,z=cz+corners[i][2]*hz;
    float dy=y-jY;y=jY+dy*cj-z*sj;z=dy*sj+z*cj;
    dy=y-rY;y=rY+dy*cr-z*sr;z=dy*sr+z*cr;
    dy=y-hip;y=hip+dy*cs-z*ss;z=dy*ss+z*cs;
    float nx=normals[f][0],ny=normals[f][1],nz=normals[f][2],t;
    t=ny*cj-nz*sj;nz=ny*sj+nz*cj;ny=t;
    t=ny*cr-nz*sr;nz=ny*sr+nz*cr;ny=t;
    t=ny*cs-nz*ss;nz=ny*ss+nz*cs;ny=t;
    out.push_back({cx+x,y,z,color.x,color.y,color.z,nx,ny,nz,0,0,float(Plain)});
   }
  };
  auto tilt=[&](size_t from){float ca=std::cos(lean),sa=std::sin(lean);for(size_t i=from;i<out.size();i++){auto&v=out[i];float dy=v.y-hip,z=v.z;v.y=hip+dy*ca-z*sa;v.z=dy*sa+z*ca;float ny=v.ny,nz=v.nz;v.ny=ny*ca-nz*sa;v.nz=ny*sa+nz*ca;}};
  const physics::Vec3 skin{.72f,.53f,.38f},hair{.15f,.12f,.09f},jacket{.24f,.31f,.22f},sleeve{.28f,.35f,.25f},pants{.16f,.19f,.15f},boots{.09f,.08f,.07f};
  size_t headBegin=out.size();
  ellipsoid(out,{0,hip+.685f+bob+breath,0},{.105f,.125f,.115f},skin);
  ellipsoid(out,{0,hip+.74f+bob+breath,.01f},{.115f,.085f,.125f},hair);
  tilt(headBegin);
  box(0,hip+.535f+bob+breath,0,.05f,.045f,.05f,skin,hip,lean,hip,0,0);   // neck
  box(0,hip+.31f+bob+breath,0,.175f,.17f,.105f,jacket,hip,lean,hip,0,0); // torso
  box(0,hip+.06f+bob,0,.155f,.08f,.095f,pants,hip,lean*.5f,hip,0,0);     // pelvis
  // Two-bone leg IK: thigh .4, shin .45, ankle .05 above the floor.
  float d=std::max(.1f,hipH-.05f);
  float alpha=std::acos(std::clamp((.16f+d*d-.2025f)/(.8f*d),-1.f,1.f));
  float gamma=std::asin(std::clamp(.4f*std::sin(alpha)/.45f,-1.f,1.f));
  for(int side:{-1,1}){
   float phase=walk+(side>0?0.f:3.14159265f),swing=std::sin(phase);
   float thigh=alpha+swing*.55f*amp;
   float knee=-(alpha+gamma)-amp*.85f*std::max(0.f,std::sin(phase-1.1f)); // heel kicks up on swing-through
   float arm=-swing*.45f*amp+std::sin(idleTime*1.9f)*.02f*(1.f-amp);
   float elbow=.12f+amp*.45f*std::max(0.f,-swing);                        // bends more when the arm swings forward
   float shoulder=hip+.46f+bob+breath,elbowY=hip+.18f+bob+breath,kneeY=hip-.4f+bob;
   float ax=side*.235f,lx=side*.1f;
   box(ax,hip+.32f+bob+breath,0,.055f,.14f,.06f,jacket,shoulder,arm,shoulder,0,lean); // upper arm
   box(ax,hip+.055f+bob+breath,0,.048f,.125f,.052f,sleeve,elbowY,elbow,shoulder,arm,lean); // forearm
   box(ax,hip-.11f+bob+breath,0,.045f,.05f,.05f,skin,elbowY,elbow,shoulder,arm,lean);     // hand
   box(lx,hip-.2f+bob,0,.08f,.2f,.09f,pants,hip,thigh,hip,0,0);           // thigh
   box(lx,hip-.6f+bob,0,.065f,.2f,.07f,pants,kneeY,knee,hip,thigh,0);     // shin
   // Boot stays flat on the ground, glued to the FK ankle position.
   float shinAbs=thigh+knee;
   float az=-.4f*std::sin(thigh)-.45f*std::sin(shinAbs);
   float ay=hip+bob-.4f*std::cos(thigh)-.45f*std::cos(shinAbs);
   box(lx,ay,az-.03f,.075f,.05f,.12f,boots,ay,0,hip,0,0);
  }
  if(prone){float ca=std::cos(-1.45f),sa=std::sin(-1.45f),py=floor+.12f;
   for(size_t i=begin;i<out.size();i++){auto&v=out[i];float dy=v.y-py,z=v.z;v.y=py+dy*ca-z*sa;v.z=dy*sa+z*ca;float ny=v.ny,nz=v.nz;v.ny=ny*ca-nz*sa;v.nz=ny*sa+nz*ca;}}
  float c=std::cos(bodyYaw),s=std::sin(bodyYaw);
  for(size_t i=begin;i<out.size();i++){auto&v=out[i];float x=v.x,z=v.z;v.x=eye.x+c*x+s*z;v.z=eye.z-s*x+c*z;x=v.nx;z=v.nz;v.nx=c*x+s*z;v.nz=-s*x+c*z;}
 }
};
}
