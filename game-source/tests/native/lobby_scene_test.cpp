#include "../../sys/android/app/src/main/cpp/LobbyScene.h"
#include <cassert>
#include <iostream>
int main(){using namespace sandbox;
 LobbyScene s;
 std::array<bool,16> drawings{};
 for(const auto&v:s.room)if(v.layer>=16){int tile=int(v.layer)-16;assert(tile>=0&&tile<16);drawings[tile]=true;assert(v.u>=-.001f&&v.u<=1.001f&&v.v>=-.001f&&v.v<=1.001f);}
 for(bool present:drawings)assert(present); // Every original drawing is actually on room geometry.
 assert(s.room.size()>10000); // Detailed native bunker, not the old empty room.
 s.update(0,0,0,0,false,false,1.f/60);assert(!s.effects.empty());auto first=s.effects.front();
 s.update(0,0,0,0,false,false,1.f/60);assert(s.effects.front().x!=first.x||s.effects.front().y!=first.y);
 for(const auto&v:s.effects)assert(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z));
 auto hit=s.hit(.5f,.5f,16.f/9);assert(hit[0]==0&&hit[1]>.49f&&hit[1]<.51f);
 for(int screen=0;screen<4;screen++){
  s.yaw=-screen*1.57079632679f;
  auto p=LobbyScene::screenPoint(screen,.5f,.5f);float nx=0,ny=0;
  s.x=s.z=0;assert(s.project(p,16.f/9,nx,ny));auto h=s.hit(nx,ny,16.f/9);
  assert(int(h[0])==screen&&std::abs(h[1]-.5f)<.001f&&std::abs(h[2]-.5f)<.001f);
 }
 s.yaw=0;s.x=s.z=0;
 for(int i=0;i<1200;i++)s.update(0,1,0,0,false,false,1.f/120);
 assert(s.z>-7.76f&&s.z<-7.5f); // Actual player capsule hits the wall.
 for(int i=0;i<120;i++)s.update(0,0,0,0,false,true,1.f/120);
 assert(s.y<1.1f);
 for(const auto&v:s.room)assert(std::isfinite(v.x)&&std::isfinite(v.nx));
 std::cout<<"Native lobby: room geometry, capsule walls/crouch, all screen ray/projection round trips OK\n";
}
