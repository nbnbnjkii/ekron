#include "../../sys/android/app/src/main/cpp/ObjectMesh.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
int main(){
 using namespace sandbox;
 static_assert(kObjectTypeCount==48);
 World world;world.objects.clear();world.walls.clear();world.wallSegments.clear();
 for(int type=13;type<kObjectTypeCount;type++){
  Object object{type,0,0,0};auto box=World::bounds(object);
  assert(box.w>0&&box.h>0&&box.d>0&&std::isfinite(World::mass(object))&&World::mass(object)>0);
  std::vector<MeshVertex> vertices;appendObject(vertices,object);
  assert(!vertices.empty()&&vertices.size()%3==0);
  for(const auto&v:vertices)assert(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z));
  world.objects.push_back(object);
 }
 const char* path="/tmp/ekron-archive-catalog-test.txt";
 assert(world.save(path));World restored;assert(restored.load(path));std::remove(path);
 assert(restored.objects.size()==35);
 for(size_t i=0;i<restored.objects.size();i++)assert(restored.objects[i].type==int(i)+13);
 std::cout<<"ZIP catalog: all added object types render and survive save/load\n";
}
