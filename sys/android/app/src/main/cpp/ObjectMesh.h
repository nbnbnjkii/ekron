#pragma once
#include "MeshGeometry.h"
namespace sandbox {
inline void appendObject(std::vector<MeshVertex>&v,const sandbox::Object&o,bool preview=false,bool valid=false){
 size_t begin=v.size();
 auto body=sandbox::World::bounds(o);
 body.material=o.type==3||o.type==4||o.type==5||o.type==6?sandbox::Wood:o.type==7?sandbox::Fabric:body.material;
 (void)preview;(void)valid; // Preview uses identical geometry/materials; hologram is a render pass.
 // Local-space furniture parts rotated by 90 degree increments.
 auto part=[&](float x,float y,float z,float w,float h,float d,float r,float g,float b,int material=-1){
  for(int i=0;i<o.rotation;i++){float old=x;x=z;z=-old;std::swap(w,d);}
  appendCube(v,{o.x+x,y+o.y,o.z+z,w,h,d,r,g,b,material>=0?material:o.type==4||o.type==5?sandbox::Wood:o.type==7?sandbox::Fabric:sandbox::Plain});
 };
 // Small smooth knobs, including normals and spherical UVs, rotated with the whole object.
 auto knob=[&](float x,float y,float z,float radius){
  constexpr int rings=6,sides=12;
  auto vertex=[&](int ring,int side){
   float a=3.14159265f*ring/rings,b=6.2831853f*side/sides;
   float nx=std::sin(a)*std::cos(b),ny=std::cos(a),nz=std::sin(a)*std::sin(b);
   float px=x+radius*nx,pz=z+radius*nz;
   for(int turn=0;turn<o.rotation;turn++){float old=px;px=pz;pz=-old;old=nx;nx=nz;nz=-old;}
   return MeshVertex{o.x+px,o.y+y+radius*ny,o.z+pz,.22f,.24f,.25f,nx,ny,nz,float(side)/sides,float(ring)/rings,float(Plain)};
  };
  for(int r=0;r<rings;r++)for(int s=0;s<sides;s++){
   for(auto vtx:{vertex(r,s),vertex(r+1,s),vertex(r+1,s+1),vertex(r,s),vertex(r+1,s+1),vertex(r,s+1)})v.push_back(vtx);
  }
 };
 auto cyl=[&](float cx,float cy,float cz,float rb,float rt,float halfY,float r,float g,float b,int material){appendFrustum(v,o.x+cx,o.y+cy,o.z+cz,rb,rt,halfY,r,g,b,material);};
 // Oriented box between arbitrary directions (hedgehog beams), rotated by the object's 90deg steps.
 auto beam=[&](float cx,float cy,float cz,float dx,float dy,float dz,float len,float t,float r,float g,float b,int material){
  for(int i=0;i<o.rotation;i++){float old=cx;cx=cz;cz=-old;old=dx;dx=dz;dz=-old;}
  float l=std::sqrt(dx*dx+dy*dy+dz*dz);dx/=l;dy/=l;dz/=l;
  float ax=0,ay=1,az=0;if(std::abs(dy)>.9f){ax=1;ay=0;}
  float sx=dy*az-dz*ay,sy=dz*ax-dx*az,sz=dx*ay-dy*ax;float sl=std::sqrt(sx*sx+sy*sy+sz*sz);sx/=sl;sy/=sl;sz/=sl;
  float tx=sy*dz-sz*dy,ty=sz*dx-sx*dz,tz=sx*dy-sy*dx;
  float hl=len/2,ht=t/2,px[8],py[8],pz[8];int k=0;
  for(int a:{-1,1})for(int b2:{-1,1})for(int c:{-1,1}){px[k]=o.x+cx+a*hl*dx+b2*ht*sx+c*ht*tx;py[k]=o.y+cy+a*hl*dy+b2*ht*sy+c*ht*ty;pz[k]=o.z+cz+a*hl*dz+b2*ht*sz+c*ht*tz;k++;}
  const int faces[6][4]={{4,5,7,6},{0,2,3,1},{2,6,7,3},{0,1,5,4},{1,3,7,5},{0,4,6,2}};
  const float normals[6][3]={{dx,dy,dz},{-dx,-dy,-dz},{sx,sy,sz},{-sx,-sy,-sz},{tx,ty,tz},{-tx,-ty,-tz}};
  for(int f=0;f<6;f++){const auto&n=normals[f];
   for(int idx:{0,1,2,0,2,3}){int i=faces[f][idx];v.push_back({px[i],py[i],pz[i],r,g,b,n[0],n[1],n[2],0,0,float(material)});}}
 };
 // Flat panel: length along a YZ direction, width along local X (tent roof, tarps).
 auto panel=[&](float cx,float cy,float cz,float dy,float dz,float len,float w,float t,float r,float g,float b,int material){
  float ux=1,uz=0;
  for(int i=0;i<o.rotation;i++){float old=cx;cx=cz;cz=-old;old=dy;dy=dz;dz=-old;old=ux;ux=uz;uz=-old;}
  float l=std::sqrt(dy*dy+dz*dz);dy/=l;dz/=l;
  float nx=-uz*dy,ny=-ux*dz,nz=ux*dy; // cross(u,d) with d=(0,dy,dz)
  float hl=len/2,hw=w/2,ht=t/2,px[8],py[8],pz[8];int k=0;
  for(int a:{-1,1})for(int b2:{-1,1})for(int c:{-1,1}){px[k]=o.x+cx+b2*hw*ux+c*ht*nx;py[k]=o.y+cy+a*hl*dy+c*ht*ny;pz[k]=o.z+cz+a*hl*dz+b2*hw*uz+c*ht*nz;k++;}
  const int faces[6][4]={{4,5,7,6},{0,2,3,1},{2,6,7,3},{0,1,5,4},{1,3,7,5},{0,4,6,2}};
  const float normals[6][3]={{0,dy,dz},{0,-dy,-dz},{ux,0,uz},{-ux,0,-uz},{nx,ny,nz},{-nx,-ny,-nz}};
  for(int f=0;f<6;f++){const auto&n=normals[f];
   for(int idx:{0,1,2,0,2,3}){int i=faces[f][idx];v.push_back({px[i],py[i],pz[i],r,g,b,n[0],n[1],n[2],0,0,float(material)});}}
 };
  if(o.type==3){
  part(0,1.15f,0,1.1f,2.3f,.045f,1,1,1,Wood);
  for(int side:{-1,1}){part(.4f,1.05f,side*.035f,.075f,.14f,.025f,.22f,.24f,.25f,Plain);knob(.4f,1.05f,side*.046f,.024f);}
 }else if(o.type==4||o.type==5){bool table=o.type==4;float w=table?1.8f:.6f,d=table?1.f:.65f,top=table?.82f:.46f;
  part(0,top,0,w,.15f,d,.54f,.4f,.27f);
  for(int a:{-1,1})for(int b:{-1,1})part(a*(w/2-.07f),top/2,b*(d/2-.07f),.1f,top,.1f,.43f,.31f,.2f);
  if(!table)part(0,.79f,-.27f,w,.6f,.12f,.54f,.4f,.27f);
 }else if(o.type==6){
  // Deep carcass with an open upper shelving bay and lower cupboard doors.
  part(0,.06f,0,1.4f,.12f,.8f,1,1,1,Wood);part(0,2.04f,0,1.4f,.12f,.8f,1,1,1,Wood);
  for(int side:{-1,1}){
   part(side*.65f,1.05f,0,.1f,1.86f,.8f,1,1,1,Wood);
   part(side*.315f,.53f,.35f,.61f,.8f,.06f,1,1,1,Wood);
   knob(side*.09f,.62f,.39f,.025f);
  }
  part(0,1.05f,-.37f,1.2f,1.86f,.06f,1,1,1,Wood);
  for(float y:{.96f,1.33f,1.69f})part(0,y,0,1.2f,.055f,.73f,1,1,1,Wood);
 }else if(o.type==2){
  for(int side:{-1,1}){part(side*.96f,1.2f,0,.08f,2.4f,.22f,.68f,.69f,.63f,sandbox::Wood);part(0,1.2f+side*1.16f,0,2,.08f,.22f,.68f,.69f,.63f,sandbox::Wood);}part(0,1.2f,0,.06f,2.4f,.2f,.68f,.69f,.63f,sandbox::Wood);part(0,1.2f,0,1.84f,2.24f,.05f,1,1,1,sandbox::Glass);
  }else if(o.type==7){part(0,.32f,0,2.3f,.6f,.95f,.33f,.4f,.36f);part(0,.78f,-.38f,2.3f,.44f,.2f,.33f,.4f,.36f);for(int a:{-1,1})part(a*1.04f,.65f,0,.22f,.4f,.95f,.33f,.4f,.36f);
  }else if(o.type==13){ // wooden shipping crate with visible slats
   part(0,.5f,0,.92f,.92f,.92f,.52f,.31f,.14f,sandbox::Wood);for(float p:{-.28f,0.f,.28f}){part(p,.5f,-.466f,.055f,.82f,.025f,.22f,.12f,.06f,sandbox::Plain);part(-.466f,.5f,p,.025f,.82f,.055f,.22f,.12f,.06f,sandbox::Plain);}
  }else if(o.type==14){ // metal barrel: rounded ribbed drum
   cyl(0,.57f,0,.36f,.30f,.55f,.20f,.25f,.27f,sandbox::Rust);
   cyl(0,.055f,0,.37f,.37f,.05f,.18f,.23f,.25f,sandbox::Rust);
   cyl(0,1.085f,0,.31f,.31f,.05f,.18f,.23f,.25f,sandbox::Rust);
  }else if(o.type==15){ // wooden pallet
   for(float x:{-.48f,0.f,.48f})part(x,.12f,0,.16f,.22f,1.05f,.48f,.28f,.12f,sandbox::Wood);
   for(float z:{-.48f,0.f,.48f})part(0,.28f,z,1.08f,.14f,.14f,.58f,.36f,.16f,sandbox::Wood);
  }else if(o.type==16){ // steel pipe bundle
   for(float y:{.06f,.16f,.26f})part(0,y,0,1.85f,.07f,.07f,.22f,.25f,.27f,sandbox::Rust);
  }else if(o.type==17){ // timber plank stack
   for(int i=0;i<3;i++)part(0,.08f+i*.12f,0,1.85f,.1f,.34f,.48f,.28f,.12f,sandbox::Wood);
  }else if(o.type==18){ // cement bag
   part(0,.35f,0,1.42f,.58f,.34f,.78f,.78f,.7f,sandbox::Concrete);part(0,.66f,0,1.28f,.08f,.3f,.68f,.68f,.6f,sandbox::Plain);
  }else if(o.type==19){ // scaffold frame
   for(float x:{-.72f,.72f})for(float z:{-.16f,.16f})part(x,.42f,z,.08f,.76f,.08f,.35f,.37f,.38f,sandbox::Rust);
   for(float y:{.08f,.76f})part(0,y,0,1.55f,.08f,.42f,.35f,.37f,.38f,sandbox::Rust);
  }else if(o.type==20){ // traffic cone: conical body + round base
   cyl(0,.06f,0,.36f,.36f,.06f,.95f,.38f,.07f,sandbox::Rust);
   cyl(0,.58f,0,.35f,0.f,.50f,1.f,.42f,.08f,sandbox::Rust);
  }else if(o.type==21){ // public waste bin (green)
   part(0,.72f,0,.62f,1.35f,.62f,.06f,.40f,.19f,sandbox::Rust);part(0,1.43f,0,.7f,.12f,.7f,.05f,.14f,.09f,sandbox::Plain);
  }else if(o.type==22){ // street bench
   part(0,.72f,0,1.65f,.14f,.45f,.42f,.25f,.1f,sandbox::Wood);part(0,1.12f,-.18f,1.65f,.65f,.12f,.42f,.25f,.1f,sandbox::Wood);for(float x:{-.62f,.62f})part(x,.35f,0,.12f,.7f,.35f,.25f,.27f,.28f,sandbox::Rust);
  }else if(o.type==23){ // manhole cover
   part(0,.08f,0,.68f,.16f,.68f,.22f,.25f,.27f,sandbox::Rust);part(0,.17f,0,.58f,.04f,.58f,.3f,.33f,.34f,sandbox::Plain);
  }else if(o.type==24){ // concrete road barrier
   part(0,.75f,0,1.72f,1.45f,.4f,.55f,.56f,.53f,sandbox::Concrete);part(0,1.05f,-.21f,1.45f,.12f,.025f,.9f,.75f,.15f,sandbox::Plain);
  }else if(o.type==25){ // loose tire: round torus (solid hub)
   cyl(0,.18f,0,.34f,.34f,.16f,.07f,.075f,.08f,sandbox::Rust);
   cyl(0,.18f,0,.20f,.20f,.165f,.05f,.05f,.055f,sandbox::Plain);
  }else if(o.type==26){ // fence post
   part(0,1.2f,0,.14f,2.3f,.14f,.3f,.32f,.33f,sandbox::Rust);
  }else if(o.type==27){ // steel beam
   part(0,.15f,0,1.9f,.25f,.25f,.25f,.28f,.3f,sandbox::Rust);part(0,.15f,-.13f,1.9f,.12f,.04f,.45f,.47f,.48f,sandbox::Plain);
  }else if(o.type==28){ // outdoor dumpster (green body, dark lid)
   part(0,.35f,0,1.05f,.7f,.7f,.07f,.44f,.22f,sandbox::Rust);part(0,.76f,0,1.15f,.12f,.78f,.06f,.18f,.12f,sandbox::Plain);
  }else if(o.type==29){ // explosive barrel: round red drum with ribs
   cyl(0,.57f,0,.34f,.30f,.55f,.82f,.04f,.02f,sandbox::Plain);
   cyl(0,.06f,0,.35f,.35f,.055f,.15f,.15f,.14f,sandbox::Plain);
   cyl(0,1.08f,0,.31f,.31f,.055f,.15f,.15f,.14f,sandbox::Plain);
  }else if(o.type==36){ // sandbag stack: three staggered rows of khaki burlap bags
   for(int row=0;row<3;row++){int n=row==1?2:3;for(int i=0;i<n;i++){float shade=(row+i)%2?.94f:1.f;
    part((i-(n-1)/2.f)*.4f,.14f+row*.24f,0,.38f,.22f,.5f,.76f*shade,.66f*shade,.47f*shade,sandbox::Plain);}}
  }else if(o.type==37){ // concertina razor wire: helical coil with barbs
   const int steps=48;float px=-.8f,py=.3f+.27f,pz=0;
   for(int i=1;i<=steps;i++){float t=i/float(steps),ang=t*4*6.2831853f;
    float x=-.8f+1.6f*t,y=.3f+.27f*std::cos(ang),z=.27f*std::sin(ang);
    beam((px+x)/2,(py+y)/2,(pz+z)/2,x-px,y-py,z-pz,std::sqrt((x-px)*(x-px)+(y-py)*(y-py)+(z-pz)*(z-pz)),.026f,.42f,.44f,.46f,sandbox::Rust);
    if(i%4==0){beam(x,y,z,0,1,1,.1f,.014f,.5f,.52f,.54f,sandbox::Plain);beam(x,y,z,0,1,-1,.1f,.014f,.5f,.52f,.54f,sandbox::Plain);}
    px=x;py=y;pz=z;}
  }else if(o.type==38){ // anti-tank hedgehog: three crossed steel beams
   beam(0,.75f,0,1,1,0,2.1f,.14f,.3f,.33f,.35f,sandbox::Rust);
   beam(0,.75f,0,-1,1,0,2.1f,.14f,.3f,.33f,.35f,sandbox::Rust);
   beam(0,.75f,0,0,.3f,1,1.8f,.14f,.35f,.38f,.4f,sandbox::Rust);
  }else if(o.type==39){ // ammo crate: olive painted box, steel straps, stencil
   part(0,.21f,0,.86f,.42f,.42f,.38f,.42f,.27f,sandbox::Plain);
   part(0,.45f,0,.9f,.08f,.45f,.33f,.37f,.23f,sandbox::Plain);
   for(float x:{-.28f,.28f})part(x,.25f,0,.05f,.5f,.46f,.25f,.27f,.24f,sandbox::Plain);
   part(0,.24f,.225f,.3f,.12f,.015f,.85f,.78f,.3f,sandbox::Plain);
   for(float x:{-.44f,.44f})part(x,.3f,0,.03f,.06f,.16f,.2f,.22f,.2f,sandbox::Plain);
  }else if(o.type==40){ // artillery shell crate: long steel box with latches
   part(0,.2f,0,1.36f,.4f,.46f,.28f,.33f,.24f,sandbox::Plain);
   part(0,.44f,0,1.4f,.08f,.5f,.24f,.29f,.2f,sandbox::Plain);
   for(float x:{-.5f,.5f}){part(x,.46f,.24f,.1f,.1f,.04f,.18f,.2f,.17f,sandbox::Plain);part(x,.2f,0,.06f,.42f,.5f,.2f,.24f,.18f,sandbox::Plain);}
   part(0,.22f,.24f,.5f,.1f,.015f,.85f,.78f,.3f,sandbox::Plain);
  }else if(o.type==41){ // dragon's tooth: concrete anti-tank pyramid
   part(0,.06f,0,1.05f,.12f,1.05f,.55f,.56f,.53f,sandbox::Concrete);
   cyl(0,.68f,0,.58f,.1f,.56f,.62f,.63f,.6f,sandbox::Concrete);
  }else if(o.type==42){ // army jerrycan: olive pressed-steel can with X rib
   part(0,.3f,0,.46f,.56f,.24f,.33f,.37f,.24f,sandbox::Plain);
   for(int side:{-1,1}){beam(0,.3f,side*.125f,1,1.2f,0,.42f,.03f,.27f,.3f,.19f,sandbox::Plain);beam(0,.3f,side*.125f,-1,1.2f,0,.42f,.03f,.27f,.3f,.19f,sandbox::Plain);}
   part(0,.6f,0,.3f,.05f,.06f,.27f,.3f,.19f,sandbox::Plain);
   for(float x:{-.14f,.14f})part(x,.575f,0,.04f,.05f,.06f,.27f,.3f,.19f,sandbox::Plain);
   cyl(.16f,.615f,.06f,.045f,.045f,.03f,.2f,.22f,.18f,sandbox::Plain);
  }else if(o.type==43){ // army tent: camo A-frame with ridge pole and rear gable
   panel(0,.84f,-.5f,1.64f,1.f,1.93f,2.4f,.04f,1,1,1,sandbox::Fabric);
   panel(0,.84f,.5f,1.64f,-1.f,1.93f,2.4f,.04f,1,1,1,sandbox::Fabric);
   for(int i=0;i<4;i++)part(-1.17f,.2f+i*.4f,0,.05f,.4f,1.76f-i*.48f,1,1,1,sandbox::Fabric);
   part(0,1.66f,0,2.44f,.07f,.07f,.3f,.32f,.3f,sandbox::Rust);
   for(float x:{-1.15f,1.15f})cyl(x,.82f,0,.035f,.035f,.82f,.3f,.32f,.3f,sandbox::Rust);
  }else if(o.type==44){ // camouflage net screen: posts with hanging camo strips
   for(float x:{-1.1f,1.1f})part(x,.95f,0,.07f,1.9f,.07f,.3f,.32f,.33f,sandbox::Rust);
   part(0,1.86f,0,2.3f,.06f,.06f,.3f,.32f,.33f,sandbox::Rust);
   for(int i=0;i<9;i++){float h=1.35f+.3f*std::sin(i*2.1f);
    part(-1.f+i*.25f,1.83f-h/2,0,.22f,h,.02f,1,1,1,sandbox::Fabric);}
  }else if(o.type==45){ // supply crate stack: two olive crates plus one khaki on top
   for(float x:{-.32f,.32f}){part(x,.26f,0,.6f,.5f,.85f,.4f,.44f,.28f,sandbox::Wood);part(x,.26f,0,.62f,.1f,.87f,.3f,.33f,.2f,sandbox::Plain);}
   part(0,.78f,0,.9f,.5f,.7f,.5f,.46f,.32f,sandbox::Wood);part(0,.78f,0,.92f,.1f,.72f,.36f,.33f,.22f,sandbox::Plain);
   part(0,.78f,.36f,.3f,.12f,.015f,.85f,.78f,.3f,sandbox::Plain);
  }else if(o.type==46){ // fuel drum: olive barrel with ribs and filler cap
   cyl(0,.55f,0,.4f,.4f,.52f,.32f,.36f,.22f,sandbox::Plain);
   for(float y:{.28f,.82f})cyl(0,y,0,.42f,.42f,.03f,.26f,.3f,.18f,sandbox::Plain);
   cyl(0,1.09f,0,.41f,.41f,.03f,.28f,.32f,.2f,sandbox::Plain);
   cyl(.2f,1.13f,0,.06f,.06f,.03f,.2f,.22f,.18f,sandbox::Plain);
   part(0,.55f,.405f,.3f,.3f,.015f,.8f,.7f,.25f,sandbox::Plain);
  }else if(o.type==47){ // field medical box: white case with red cross
   part(0,.18f,0,.66f,.34f,.42f,.88f,.88f,.86f,sandbox::Plain);
   part(0,.37f,0,.68f,.06f,.44f,.8f,.8f,.78f,sandbox::Plain);
   part(0,.405f,0,.3f,.015f,.09f,.8f,.12f,.1f,sandbox::Plain);
   part(0,.405f,0,.09f,.015f,.3f,.8f,.12f,.1f,sandbox::Plain);
   part(0,.2f,.215f,.24f,.08f,.015f,.8f,.12f,.1f,sandbox::Plain);
   part(0,.2f,.215f,.08f,.24f,.015f,.8f,.12f,.1f,sandbox::Plain);
   for(float x:{-.34f,.34f})part(x,.3f,0,.02f,.06f,.16f,.3f,.3f,.32f,sandbox::Plain);
  }else appendCube(v,body);
 int surface=World::fixedSurface(o.type);
 if(surface>=0)for(size_t i=begin;i<v.size();i++){if(v[i].layer!=Plain||(o.type!=3&&o.type!=6)){v[i].layer=float(surface);if(o.type<13||o.type>=30)v[i].r=v[i].g=v[i].b=1;}}
}
}
