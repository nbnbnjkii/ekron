#include "../../sys/android/app/src/main/cpp/GltfScene.h"
#include "../../sys/android/app/src/main/cpp/LevelPlacement.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <chrono>
#include "../../sys/android/app/src/main/cpp/PlayerPhysics.h"
#include "fixtures/LegacyLevelPlacement.h"
int main(){
 // Rasterization must produce the same regions and clearance choices as brute force.
 for(int trial=0;trial<6;trial++){
  sandbox::World w;w.walls.clear();w.wallSegments.clear();w.objects.clear();
  w.walls.push_back({-3,0,2,2,2,5,0,0,0});
  w.objects.push_back({5,trial%4,3,-2});
  w.wallSegments.push_back({-8,-7,7,float(trial)-3});
  w.wallSegments.push_back({-2,7,8,7});
  std::array<float,3> lo{-10,0,-10},hi{10,5,10};
  city::SpawnPoint a;legacy_city::SpawnPoint b;
  std::vector<std::array<float,2>> ar,br;
  bool ok=city::findRoadSpawn(w,lo,hi,1,1,a,&ar);
  assert(ok==legacy_city::findRoadSpawn(w,lo,hi,1,1,b,&br));
  assert(ok&&a.x==b.x&&a.z==b.z&&a.reachableCells==b.reachableCells&&ar==br);
 }
 auto started=std::chrono::steady_clock::now();

 city::Scene scene;std::string log;city::LoadOptions options;options.targetExtent=4000;options.preferredSpawnX=24;options.preferredSpawnZ=104;
 int last=0,updates=0;options.progress=[&](int p){assert(p>=last&&p<=100);last=p;updates++;};
 auto read=[](const std::string&p,std::vector<unsigned char>&b){std::ifstream f(p,std::ios::binary);if(!f)return false;b.assign(std::istreambuf_iterator<char>(f),{});return true;};
 assert(scene.load("sys/android/app/src/main/assets/models/military_city.glb",read,log,options));
 assert(last==100&&updates>=6);
 assert(scene.batches.size()>50&&scene.batches.size()<200);
 assert(scene.materials.size()>100&&scene.images.size()>=4);
 size_t owners=0;for(size_t i=0;i<scene.materials.size();i++){auto owner=city::sharedTextureOwner(scene.materials,i);assert(owner<=i);if(owner==i)owners++;}
 assert(owners<20); // Military materials reuse their source textures.
 {std::vector<city::Material> m(3);m[0].image=m[1].image=m[2].image=0;m[1].color[0]=.2f;m[2].wrapS=33071;
 assert(city::sharedTextureOwner(m,1)==0);assert(city::sharedTextureOwner(m,2)==2);}
 std::cout<<"Unique GPU texture storage: "<<owners<<" / "<<scene.materials.size()<<" materials\n";
 assert(scene.maximum[0]-scene.minimum[0]>3999);
 size_t triangles=0;bool colored=false;
 for(const auto&batch:scene.batches){triangles+=batch.vertices.size()/3;for(const auto&v:batch.vertices)if(v.r!=1||v.g!=1||v.b!=1)colored=true;}
 assert(triangles>300000&&triangles<400000&&colored);
 const auto&image=scene.images[0];assert(image.width>0&&image.height>0);
 bool varied=false;for(size_t i=4;i<image.rgba.size();i+=4)if(image.rgba[i]!=image.rgba[0]){varied=true;break;}assert(varied);
 assert(scene.collision&&scene.collision->size()>300000);
 physics::PlayerController capsule;float floor=scene.collision->groundHeight(24,104,3,-2);
 assert(std::isfinite(floor)&&!capsule.overlaps(*scene.collision,24,floor+1.701f,104));
 sandbox::World world;world.walls=scene.colliders;world.wallSegments=scene.wallSegments;
 world.x=scene.spawnX;world.z=scene.spawnZ;
 assert(!world.blocked({world.x,0,world.z,.62f,2,.62f,0,0,0}));
 city::SpawnPoint spawn;assert(city::findRoadSpawn(world,scene.minimum,scene.maximum,world.x,world.z,spawn));
 float x=0,z=0;assert(city::findCarParking(world,scene.minimum,scene.maximum,spawn.x+3,spawn.z-9,x,z));
 // Recovery must work away from the origin on the kilometre-wide map, too.
 assert(city::findRoadSpawn(world,scene.minimum,scene.maximum,300,300,spawn));
 std::cout<<"Load + navigation seconds: "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<"\n";
 std::cout<<log<<" triangles="<<triangles<<" parking="<<x<<","<<z<<"\n";
}
