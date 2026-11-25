#include "../../sys/android/app/src/main/cpp/GltfScene.h"
#include <fstream>
#include <iostream>
#include <cassert>
int main(){
 auto read=[](const std::string&p,std::vector<unsigned char>&out){std::ifstream f(p,std::ios::binary);if(!f)return false;out.assign(std::istreambuf_iterator<char>(f),{});return true;};
 for(auto name:{"chasing_m2_rc_underwater_drone_remote.glb","fpv_drone.glb"}){
  city::Scene scene;city::LoadOptions options;options.animateRotors=std::string(name)=="fpv_drone.glb";options.level=false;options.buildPhysics=false;options.targetExtent=.38f;std::string log;
  assert(scene.load(std::string("sys/android/app/src/main/assets/models/")+name,read,log,options));
  size_t triangles=0;for(const auto&b:scene.batches){triangles+=b.vertices.size()/3;assert(b.material>=0);}
  size_t rotors=0;for(const auto&b:scene.batches)if(b.rotor){rotors++;std::array<float,3> lo{1e6f,1e6f,1e6f},hi{-1e6f,-1e6f,-1e6f};for(const auto&v:b.vertices){std::array<float,3> p{v.x,v.y,v.z};for(int i=0;i<3;i++){lo[i]=std::min(lo[i],p[i]);hi[i]=std::max(hi[i],p[i]);}}std::cout<<"Rotor extents "<<hi[0]-lo[0]<<" "<<hi[1]-lo[1]<<" "<<hi[2]-lo[2]<<"\n";assert(hi[1]-lo[1]<.02f);}
  assert(rotors==(options.animateRotors?4:0));
  assert(triangles>0&&!scene.materials.empty());
  float extent=0;for(int i=0;i<3;i++){assert(std::isfinite(scene.minimum[i])&&std::isfinite(scene.maximum[i]));extent=std::max(extent,scene.maximum[i]-scene.minimum[i]);}
  assert(extent>0);
  assert(std::abs(std::max(scene.maximum[0]-scene.minimum[0],scene.maximum[2]-scene.minimum[2])-.38f)<.001f);
  std::cout<<name<<": "<<triangles<<" triangles, "<<scene.materials.size()<<" materials, "<<scene.images.size()<<" images; bounds ";
  for(int i=0;i<3;i++){std::cout<<scene.minimum[i]<<":"<<scene.maximum[i]<<" ";}std::cout<<"\n";
 }
}
