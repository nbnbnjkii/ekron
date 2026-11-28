#pragma once
#include "MeshGeometry.h"
#include "CollisionMesh.h"
namespace sandbox {
// Native adaptation of military-lobby-ui-design/src/game/scene.ts.
// Local material tags are used ONLY by LobbyRenderer, never by the game world.
struct LobbyDecor {
 std::vector<MeshVertex>&out;physics::CollisionMesh&collision;
 static constexpr float pi=3.14159265359f;
 void box(Box b,int material=1,bool solid=false){
  size_t start=out.size();appendCube(out,b);for(size_t i=start;i<out.size();i++)out[i].layer=float(material);
  if(solid)collision.addBox({b.x-b.w/2,b.y-b.h/2,b.z-b.d/2},{b.x+b.w/2,b.y+b.h/2,b.z+b.d/2});
 }
 static physics::Vec3 wallPoint(int wall,float x,float y,float depth){
  switch(wall){case 0:return {x,y,-8+depth};case 1:return {8-depth,y,x};case 2:return {-x,y,8-depth};default:return {-8+depth,y,-x};}
 }
 void wallBox(int wall,Box b,int material=1){
  auto p=wallPoint(wall,b.x,b.y,b.z);b.x=p.x;b.y=p.y;b.z=p.z;if(wall%2)std::swap(b.w,b.d);box(b,material);
 }
 void cylinder(physics::Vec3 p,float radius,float height,physics::Vec3 color,int material=1,int segments=20){
  for(int i=0;i<segments;i++){
   float a=2*pi*i/segments,b=2*pi*(i+1)/segments;
   auto vertex=[&](float angle,float yy){return MeshVertex{p.x+radius*std::cos(angle),p.y+yy,p.z+radius*std::sin(angle),color.x,color.y,color.z,std::cos(angle),0,std::sin(angle),angle/(2*pi),yy/height+.5f,float(material)};};
   auto v0=vertex(a,-height/2),v1=vertex(b,-height/2),v2=vertex(b,height/2),v3=vertex(a,height/2);
   for(auto v:{v0,v1,v2,v0,v2,v3})out.push_back(v);
   for(float side:{-1.f,1.f}){auto centre=MeshVertex{p.x,p.y+side*height/2,p.z,color.x,color.y,color.z,0,side,0,.5f,.5f,float(material)};
    auto left=vertex(a,side*height/2),right=vertex(b,side*height/2);left.nx=right.nx=left.nz=right.nz=0;left.ny=right.ny=side;
    out.push_back(centre);out.push_back(left);out.push_back(right);}
  }
 }
 void ring(physics::Vec3 centre,float radius,float thickness,physics::Vec3 color,float tilt=0,float spin=0,int segments=64){
  auto point=[&](float a,float r){physics::Vec3 p{std::cos(a)*r,0,std::sin(a)*r};float y=-std::sin(tilt)*p.z;p.z*=std::cos(tilt);float x=p.x;p.x=std::cos(spin)*x+std::sin(spin)*p.z;p.z=-std::sin(spin)*x+std::cos(spin)*p.z;
   return MeshVertex{centre.x+p.x,centre.y+y,centre.z+p.z,color.x,color.y,color.z,0,1,0,0,0,2};};
  for(int i=0;i<segments;i++){float a=2*pi*i/segments,b=2*pi*(i+1)/segments;auto v0=point(a,radius-thickness),v1=point(b,radius-thickness),v2=point(b,radius+thickness),v3=point(a,radius+thickness);for(auto v:{v0,v1,v2,v0,v2,v3})out.push_back(v);}
 }
 void crate(float x,float z,float w,float h,float d,float bottom=0){
  box({x,bottom+h/2,z,w,h,d,.31f,.36f,.22f,Plain},6,true);
  for(float side:{-1.f,1.f}){box({x+side*w*.4f,bottom+h/2,z,.07f,h+.02f,d+.025f,.12f,.16f,.10f,Plain});box({x,bottom+h/2,z+side*d/2,w,.06f,.025f,.13f,.17f,.11f,Plain});}
  box({x,bottom+h-.02f,z,w+.03f,.05f,d+.03f,.12f,.16f,.1f,Plain});
 }
 void build(){
  box({0,-.2f,0,16,.4f,16,.34f,.35f,.31f,Plain},3,true);
  box({0,5.2f,0,16.6f,.4f,16.6f,.19f,.22f,.20f,Plain},1,true);
  for(int wall=0;wall<4;wall++){
   auto p=wallPoint(wall,0,2.5f,-.15f);box({p.x,p.y,p.z,wall%2?.3f:16.6f,5,wall%2?16.6f:.3f,.36f,.39f,.32f,Plain},0,true);
   wallBox(wall,{0,.07f,.013f,16,.14f,.02f,.65f,.48f,.12f,Plain},9);
   wallBox(wall,{0,1.2f,.04f,16,.07f,.07f,.12f,.16f,.13f,Plain});
   for(float yy:{.2f,4.f})wallBox(wall,{0,yy,.08f,6.8f,.2f,.14f,.10f,.14f,.12f,Plain});
   for(float xx:{-3.3f,3.3f})wallBox(wall,{xx,2.1f,.08f,.2f,3.6f,.14f,.10f,.14f,.12f,Plain});
   for(float yy:{.27f,3.93f})wallBox(wall,{0,yy,.151f,6.5f,.022f,.012f,.25f,.95f,.55f,Plain},2);
   for(float xx:{-3.23f,3.23f})wallBox(wall,{xx,2.1f,.151f,.022f,3.7f,.012f,.25f,.95f,.55f,Plain},2);
   wallBox(wall,{0,.012f,.34f,7,.02f,.06f,.2f,.85f,.45f,Plain},2);
   wallBox(wall,{0,4.72f,.22f,16,.16f,.16f,.24f,.30f,.21f,Plain});
   wallBox(wall,{0,4.91f,.18f,16,.09f,.09f,.63f,.49f,.12f,Plain});
   for(float x:{-7.f,-5.f,-3.f,-1.f,1.f,3.f,5.f,7.f})wallBox(wall,{x,4.8f,.15f,.08f,.36f,.24f,.1f,.14f,.11f,Plain});
   for(float x:{-4.5f,4.5f}){
    wallBox(wall,{x,3.75f,.22f,.36f,.3f,.21f,.07f,.1f,.08f,Plain});
    wallBox(wall,{x,3.75f,.33f,.26f,.20f,.02f,1.f,.56f,.18f,Plain},2);
    for(float dx:{-.09f,0.f,.09f})wallBox(wall,{x+dx,3.75f,.35f,.012f,.27f,.02f,.13f,.12f,.09f,Plain});
   }
   // Stencilled banner and opposite map board / electrical cabinet.
   float bannerX=wall<2?5.7f:-5.7f;
   wallBox(wall,{bannerX,3.15f,.12f,1.25f,3.12f,.025f,.39f,.10f,.07f,Plain},4);
   float other=-bannerX;
   if(wall==0||wall==2){wallBox(wall,{other,2.55f,.12f,2.25f,1.65f,.12f,.09f,.12f,.08f,Plain});wallBox(wall,{other,2.55f,.19f,2.12f,1.52f,.025f,.56f,.57f,.37f,Plain},5);}
   else{wallBox(wall,{other,1.f,.2f,1.1f,1.6f,.36f,.27f,.32f,.23f,Plain},10);for(int k=0;k<3;k++)wallBox(wall,{other-.3f+k*.13f,1.72f,.39f,.05f,.05f,.02f,k==2?1.f:.25f,k==0?1.f:.6f,.15f,Plain},2);}
  }
  for(float x:{-6.f,-2.f,2.f,6.f}){box({x,4.97f,0,.32f,.05f,16,.17f,.21f,.18f,Plain});box({x,4.63f,0,.32f,.05f,16,.17f,.21f,.18f,Plain});box({x,4.8f,0,.06f,.35f,16,.14f,.18f,.15f,Plain});}
  for(float z:{-5.3f,5.3f}){box({0,4.35f,z,16,.45f,.75f,.31f,.36f,.33f,Plain});for(int x=-7;x<=7;x+=2)box({float(x),4.35f,z,.07f,.52f,.82f,.16f,.20f,.17f,Plain});}
  for(float z:{-3.6f,3.6f})for(float x:{-3.6f,0.f,3.6f}){box({x,4.57f,z,1.5f,.1f,.3f,.07f,.10f,.08f,Plain});box({x,4.505f,z,1.34f,.03f,.2f,1.f,.91f,.72f,Plain},2);}
  for(float side:{-1.f,1.f}){crate(side*6.8f,5.5f,1.3f,1.1f,1.3f);crate(side*6.8f,5.5f,1.f,.3f,1.f,1.1f);crate(side*4.9f,7,.7f,.7f,.7f);}
  for(int i=0;i<3;i++){float x=-7.f+(i==1?.75f:0),z=-6.9f+(i==2?.75f:0);cylinder({x,.475f,z},.3f,.95f,{.31f,.36f,.22f},8);collision.addBox({x-.3f,0,z-.3f},{x+.3f,.95f,z+.3f});}
  for(float side:{-1.f,1.f})for(int layer=0;layer<3;layer++){
   for(int k=0;k<4;k++)box({side*(5.3f+k*.56f+(layer%2)*.15f),.11f+layer*.21f,-side*7.25f,.54f,.21f,.36f,.48f,.44f,.30f,Plain},7,true);
   for(int k=0;k<3;k++)box({side*7.25f,.11f+layer*.21f,-side*(6.6f-k*.56f),.36f,.21f,.54f,.48f,.44f,.30f,Plain},7,true);
  }
  cylinder({0,.04f,0},1.2f,.08f,{.07f,.12f,.09f});collision.addBox({-1.1f,0,-1.1f},{1.1f,.12f,1.1f});
  ring({0,.10f,0},1.03f,.022f,{.3f,1.f,.6f});ring({0,.11f,0},.62f,.012f,{.3f,.9f,.5f});
 }
 void hologram(float time){
  for(int i=0;i<84;i++){
   float seed=i*2.399963f,x=std::sin(seed)*7.3f+std::sin(time*.3f+seed)*.12f,z=std::cos(seed*1.71f)*7.3f;
   float y=.2f+std::fmod(i*.37f+time*.045f,4.4f),r=.013f;
   for(int axis=0;axis<2;axis++){
    auto vertex=[&](float u,float v){return MeshVertex{x+(axis==0?u:0),y+v,z+(axis==1?u:0),.48f,.57f,.46f,0,1,0,0,0,11};};
    auto a=vertex(-r,-r),b=vertex(r,-r),c=vertex(r,r),d=vertex(-r,r);for(auto v:{a,b,c,a,c,d})out.push_back(v);
   }
  }
  auto centre=physics::Vec3{0,1.22f+std::sin(time*1.2f)*.03f,0};
  for(int latitude=-3;latitude<=3;latitude++){float a=latitude*pi/8;auto p=centre;p.y+=std::sin(a)*.55f;ring(p,std::cos(a)*.55f,.003f,{.2f,.75f,.45f},0,time*.25f,40);}
  for(int k=0;k<6;k++)ring(centre,.55f,.003f,{.2f,.75f,.45f},pi/2,time*.25f+k*pi/6,40);
  for(int k=0;k<3;k++)ring(centre,.66f+k*.11f,.006f,{.65f,1.f,.76f},.4f+k*.4f,time*(k%2?-.6f:.45f),64);
 }
};
}
