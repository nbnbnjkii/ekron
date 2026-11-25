// Offline software thumbnails for every procedural catalog item, sharing the SAME
// ObjectMesh + SandboxWorld materials that Android renders in-game. The UAZ (type 8)
// keeps its existing GLB-based thumbnail; only procedural types need re-rendering.
// Build: g++ -std=c++17 -O2 tools/render_props_catalog.cpp -o /tmp/render_props
// Run:   /tmp/render_props /tmp/props ; convert each PPM to WebP with ImageMagick.
#include "../sys/android/app/src/main/cpp/ObjectMesh.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../sys/android/app/src/main/cpp/third_party/stb_image.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <array>
#include <algorithm>
#include <vector>
using sandbox::MeshVertex;
struct Image{int w=0,h=0;std::vector<unsigned char> rgb;};
int main(int argc,char**argv){
 if(argc!=2)return 1;
 std::filesystem::create_directories(argv[1]);
 const std::string root="sys/android/app/src/main/assets/";
 const char*texNames[]={"asphalt","concrete","brick","rust","plain","wood","fabric","glass"};
 std::vector<Image> images(8);
 for(int i=0;i<8;i++)if(i!=4){int n;std::ifstream f(root+"textures/"+texNames[i]+".png",std::ios::binary);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)),{});auto*p=stbi_load_from_memory(bytes.data(),int(bytes.size()),&images[i].w,&images[i].h,&n,3);if(!p)return 2;images[i].rgb.assign(p,p+images[i].w*images[i].h*3);stbi_image_free(p);}
 const struct Item{int type;const char*name;}items[]={
  {0,"wall"},{1,"floor"},{2,"window"},{3,"door"},{4,"table"},{5,"chair"},{6,"cabinet"},{7,"sofa"},
  {9,"wood_block"},{10,"brick_block"},{11,"metal_block"},{12,"cube"},
  {13,"crate"},{14,"barrel"},{15,"pallet"},{16,"pipes"},{17,"plank"},{18,"cement_bag"},
  {19,"scaffold"},{20,"cone"},{21,"bin"},{22,"bench"},{23,"manhole"},{24,"barrier"},
  {25,"tire"},{26,"post"},{27,"beam"},{28,"dumpster"},{29,"explosive_barrel"},
  {30,"crate_wood_small"},{31,"crate_wood_large"},{32,"crate_metal_small"},
  {33,"crate_metal_large"},{34,"block_small"},{35,"block_large"},
  {36,"sandbags"},{37,"razor_wire"},{38,"hedgehog"},{39,"ammo_crate"},
  {40,"shell_crate"},{41,"dragon_tooth"},{42,"jerrycan"},{43,"tent"},
  {44,"camo_net"},{45,"supply_stack"},{46,"fuel_drum"},{47,"medkit"}
 };
 for(const auto&item:items){
  std::vector<MeshVertex> mesh;
  sandbox::appendObject(mesh,{item.type,0,0,0});
  auto project=[](const MeshVertex&v){return std::array<float,3>{.82f*v.x+.57f*v.z,v.y*.88f-v.z*.39f+v.x*.27f,-.50f*v.x+.46f*v.y+.72f*v.z};};
  float minX=1e6f,maxX=-1e6f,minY=1e6f,maxY=-1e6f;
  for(const auto&v:mesh){auto p=project(v);minX=std::min(minX,p[0]);maxX=std::max(maxX,p[0]);minY=std::min(minY,p[1]);maxY=std::max(maxY,p[1]);}
  constexpr int S=320;float scale=272/std::max(maxX-minX,maxY-minY);
  std::vector<unsigned char> pixels(S*S*3);std::vector<float> depth(S*S,-1e9f);
  for(int y=0;y<S;y++)for(int x=0;x<S;x++)for(int c=0;c<3;c++)pixels[(y*S+x)*3+c]=static_cast<unsigned char>(c==0?30:c==1?40:46);
  for(size_t i=0;i+2<mesh.size();i+=3){
   std::array<float,3> p[3];for(int j=0;j<3;j++){p[j]=project(mesh[i+j]);p[j][0]=160+(p[j][0]-(minX+maxX)/2)*scale;p[j][1]=160-(p[j][1]-(minY+maxY)/2)*scale;}
   float area=(p[1][0]-p[0][0])*(p[2][1]-p[0][1])-(p[1][1]-p[0][1])*(p[2][0]-p[0][0]);if(std::abs(area)<.00001f)continue;
   int xa=std::max(0,int(std::floor(std::min({p[0][0],p[1][0],p[2][0]})))),xb=std::min(S-1,int(std::ceil(std::max({p[0][0],p[1][0],p[2][0]}))));
   int ya=std::max(0,int(std::floor(std::min({p[0][1],p[1][1],p[2][1]})))),yb=std::min(S-1,int(std::ceil(std::max({p[0][1],p[1][1],p[2][1]}))));
   const auto&v=mesh[i];int layer=int(v.layer);const auto&t=images[layer];
   float shade=.63f+.37f*std::abs(v.nx*.35f+v.ny*.82f+v.nz*.45f);
   for(int y=ya;y<=yb;y++)for(int x=xa;x<=xb;x++){
    float a=((p[1][0]-x)*(p[2][1]-y)-(p[1][1]-y)*(p[2][0]-x))/area;
    float b=((p[2][0]-x)*(p[0][1]-y)-(p[2][1]-y)*(p[0][0]-x))/area;float c=1-a-b;
    if(a<0||b<0||c<0)continue;
    if(item.type==2&&layer==7&&(x+y*2)%4!=0)continue;
    float z=a*p[0][2]+b*p[1][2]+c*p[2][2];int index=y*S+x;if(z<=depth[index])continue;depth[index]=z;
    float u=a*mesh[i].u+b*mesh[i+1].u+c*mesh[i+2].u,w=a*mesh[i].v+b*mesh[i+1].v+c*mesh[i+2].v;
    int tx=t.w?int((u-std::floor(u))*t.w)%t.w:0,ty=t.h?int((w-std::floor(w))*t.h)%t.h:0;
    const float tint[]={v.r,v.g,v.b};for(int channel=0;channel<3;channel++){float sample=t.rgb.empty()?255:t.rgb[(ty*t.w+tx)*3+channel];pixels[index*3+channel]=static_cast<unsigned char>(std::clamp(sample*tint[channel]*shade,0.f,255.f));}
   }
  }
  std::ofstream out(std::string(argv[1])+"/"+item.name+".ppm",std::ios::binary);out<<"P6\n"<<S<<" "<<S<<"\n255\n";out.write(reinterpret_cast<char*>(pixels.data()),pixels.size());
  std::cout<<item.name<<": "<<mesh.size()/3<<" triangles\n";
 }
 return 0;
}