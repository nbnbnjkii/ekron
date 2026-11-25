#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace sandbox {
enum Material { Asphalt=0, Concrete=1, Brick=2, Rust=3, Plain=4, Wood=5, Fabric=6, Glass=7 };
// Keep the original numeric IDs stable: saved worlds and the Android catalog
// use these values as their wire format. New catalog entries append to this
// range instead of renumbering existing furniture and vehicles.
static constexpr int kObjectTypeCount=48;
struct Box { float x,y,z,w,h,d,r,g,b; int material=Concrete; };
struct WheelState {float compression=0,load=0;bool grounded=false;};
struct Object { int type,rotation; float x,z; float y=0; float vx=0,vy=0,vz=0,tiltX=0,tiltZ=0,yaw=0;bool awake=true,anchored=false;int surface=-1;float doorAngle=0,doorTarget=0;bool frozen=false;float spinX=0,spinZ=0;float supportCheckTime=0;float restTime=0;float embeddedTime=0;bool destroyed=false;bool floorRecoveryUsed=false;bool edgeKick=false;std::array<WheelState,4> wheels{}; };
struct WallSegment {float ax,az,bx,bz;};
inline bool intersectsWall(const Box &box,const WallSegment &wall){
    // Slab test against a narrow 5 cm wall; respects diagonal walls and door gaps.
    float enter=0,exit=1;
    auto slab=[&](float origin,float delta,float center,float half){
        if(std::abs(delta)<.000001f)return std::abs(origin-center)<=half;
        float a=(center-half-origin)/delta,b=(center+half-origin)/delta;
        if(a>b)std::swap(a,b);
        enter=std::max(enter,a);exit=std::min(exit,b);return enter<=exit;
    };
    return slab(wall.ax,wall.bx-wall.ax,box.x,box.w/2+.025f)&&slab(wall.az,wall.bz-wall.az,box.z,box.d/2+.025f);
}
class World {
public:
    std::vector<Box> scenery, walls;
    std::vector<WallSegment> wallSegments;
    std::vector<Object> objects;
    float x=0,y=1.7f,z=19,yaw=0,pitch=0;
    int selected=0,rotation=0,surface=-1;
    static constexpr int limit=500;
    World() {
        scenery.push_back({0,-.2f,0,140,.4f,140,.32f,.37f,.31f});
        scenery.push_back({0,.01f,0,13,.04f,135,1.f,1.f,1.f,Asphalt});
        for(int side:{-1,1}) scenery.push_back({side*8.f,.08f,0,2.5f,.16f,130,.48f,.49f,.46f});
        int n=0;
        for(float bz:{-43.f,-23.f,17.f,39.f}) for(float bx:{-20.f,20.f}) building(bx,bz,n++%3!=0);
        for(int i=0;i<8;i++) {
            float cx=i%2?5.f:-5.f,cz=-49+i*13.f;
            solid({cx,.55f,cz,1.9f,1.1f,3.8f,.9f,.9f,.9f,Rust});
            scenery.push_back({cx,1.3f,cz,1.6f,.6f,1.7f,.18f,.24f,.25f,Plain});
        }
    }
    static Box bounds(const Object &o) {
        static const float dims[kObjectTypeCount][3]={
            {3,2.8f,.25f},{3,.18f,3},{2,2.4f,.22f},{1.1f,2.3f,.14f},
            {1.8f,.9f,1},{.6f,1.05f,.65f},{1.4f,2.1f,.84f},{2.3f,1,.95f},
            {4.025f,1.92485f,1.89738f},{3,2.8f,.25f},{3,2.8f,.25f},{3,2.8f,.25f},{1,1,1},
            {1.0f,1.0f,1.0f}, {.72f,1.14f,.72f}, {1.1f,.35f,1.1f}, {2.0f,.32f,.32f},
            {1.9f,.42f,.45f}, {1.42f,.70f,.34f}, {1.6f,.84f,.5f}, {.75f,1.0f,.75f},
            {.7f,1.49f,.7f}, {1.8f,1.45f,.5f}, {.72f,.20f,.72f}, {1.8f,1.45f,.45f},
            {.8f,.45f,.68f}, {.18f,2.4f,.18f}, {2.0f,.3f,.3f}, {1.15f,.82f,.8f},
            {.72f,1.14f,.72f},{.6f,.6f,.6f},{1.4f,1.4f,1.4f},{.7f,.7f,.7f},{1.3f,1.3f,1.3f},{1.5f,.5f,.6f},{2.4f,.8f,.8f},
            {1.2f,.75f,.55f},{1.7f,.65f,.65f},{1.5f,1.5f,1.5f},{.9f,.5f,.45f},{1.4f,.5f,.5f},{1.1f,1.25f,1.1f},
            {.5f,.65f,.28f},{2.4f,1.7f,2.f},{2.4f,1.9f,.3f},{1.3f,1.15f,.9f},{.85f,1.15f,.85f},{.7f,.45f,.45f}
        };
        int type=std::clamp(o.type,0,kObjectTypeCount-1);
        auto p=dims[type];
        int material=type==9||type==13||type==15||type==17||type==22?Wood:type==10?Brick:type==11||type==14||type==16||type==19||type==20||type==21||type==23||type==25||type==26||type==27||type==28?Rust:type==0||type==1||type==12||type==18||type==24?Concrete:Plain;
        return {o.x,o.y+p[1]/2,o.z,o.rotation%2?p[2]:p[0],p[1],o.rotation%2?p[0]:p[2],.5f,.43f,.32f,material};
    }
    static float mass(const Object&o) {
        if(o.type==8)return 1500.f;
        if(o.type==5)return 8.f;
        if(o.type==4)return 35.f;
        if(o.type==6)return 65.f;
        if(o.type==7)return 75.f;
        if(o.type==14)return 28.f;
        if(o.type==15)return 18.f;
        if(o.type==16)return 12.f;
        if(o.type==19)return 45.f;
        if(o.type==20)return 22.f;
        if(o.type==21)return 18.f;
        if(o.type==23)return 9.f;
        if(o.type==24)return 180.f;
        if(o.type==25)return 14.f;
        if(o.type==36)return 60.f;
        if(o.type==37)return 15.f;
        if(o.type==38)return 120.f;
        if(o.type==39)return 24.f;
        if(o.type==40)return 48.f;
        if(o.type==41)return 650.f;
        if(o.type==42)return 22.f;
        if(o.type==43)return 85.f;
        if(o.type==44)return 30.f;
        if(o.type==45)return 75.f;
        if(o.type==46)return 95.f;
        if(o.type==47)return 12.f;
        return o.type>=13?50.f:90.f;
    }
    static int fixedSurface(int type) {
        // Models that own their materials: window (glass + wood), the GLB UAZ and
        // the plain-painted red explosive barrel. Everything else has one fixed surface.
        if(type==2||type==8||type==29)return -1;
        // Military props (crates, tents, fuel drums...) paint their own parts.
        if(type>=36)return -1;
        if(type==11||type==14||type==16||type==19||type==20||type==21||type==23||type==25||type==26||type==27||type==28||type==32||type==33)return Rust;
        if(type==9||type==3||type==4||type==5||type==6||type==13||type==15||type==17||type==22||type==30||type==31)return Wood;
        if(type==10)return Brick;
        if(type==7)return Fabric;
        return Concrete; // 0,1,12,18,24,34,35
    }
    static bool overlap(const Box&a,const Box&b) {
        return std::abs(a.x-b.x)<(a.w+b.w)/2 && std::abs(a.z-b.z)<(a.d+b.d)/2;
    }
    bool blocked(const Box &b) const {
        for(const auto &w:walls) if(overlap(b,w)) return true;
        for(const auto &w:wallSegments) if(intersectsWall(b,w)) return true;
        for(const auto &o:objects) if(!o.destroyed&&overlap(b,bounds(o))) return true;
        return false;
    }
    void move(float right,float forward,float dt) {
        const float magnitude=std::max(1.f,std::hypot(right,forward));
        const float dx=(std::cos(yaw)*right-std::sin(yaw)*forward)/magnitude*5*dt;
        const float dz=(-std::sin(yaw)*right-std::cos(yaw)*forward)/magnitude*5*dt;
        if(std::abs(x+dx)<64&&!blocked({x+dx,0,z,.6f,2,.6f,0,0,0})) x+=dx;
        if(std::abs(z+dz)<64&&!blocked({x,0,z+dz,.6f,2,.6f,0,0,0})) z+=dz;
    }
    Object preview() const { return {selected,rotation,std::round((x-std::sin(yaw)*4)*2)/2,std::round((z-std::cos(yaw)*4)*2)/2}; }
    bool canPlace(const Object &o) const {
        if(o.type<0||o.type>=kObjectTypeCount||o.rotation<0||o.rotation>=4||!std::isfinite(o.x)||!std::isfinite(o.z)||!std::isfinite(o.y)||std::abs(o.y)>200||std::abs(o.x)>62||std::abs(o.z)>62) return false;
        auto b=bounds(o);
        return !blocked(b)&&!overlap(b,{x,0,z,.7f,2,.7f,0,0,0});
    }
    bool place() { const auto o=preview();if(objects.size()>=limit||!canPlace(o))return false;objects.push_back(o);return true; }
    static float rayDistance(const Object&o,float ox,float oy,float oz,float dx,float dy,float dz,float limit){
        auto b=bounds(o);
        float x=ox-b.x,y=oy-b.y,z=oz-b.z;
        auto local=[&](float &a,float &v,float &c){
            float u=std::cos(o.tiltZ)*a+std::sin(o.tiltZ)*v;
            float w=-std::sin(o.tiltZ)*a+std::cos(o.tiltZ)*v;
            a=u;v=std::cos(o.tiltX)*w+std::sin(o.tiltX)*c;c=-std::sin(o.tiltX)*w+std::cos(o.tiltX)*c;
        };
        local(x,y,z);local(dx,dy,dz);
        if(o.type==3&&std::abs(o.doorAngle)>.0001f){
            float hx=-.55f,hz=0;for(int i=0;i<o.rotation;i++){float old=hx;hx=hz;hz=-old;}
            float a=x-hx,c=z-hz,cosine=std::cos(o.doorAngle),sine=std::sin(o.doorAngle);
            x=hx+cosine*a-sine*c;z=hz+sine*a+cosine*c;
            float old=dx;dx=cosine*dx-sine*dz;dz=sine*old+cosine*dz;
        }
        float lo=0,hi=limit;
        auto slab=[&](float p,float d,float half){if(std::abs(d)<1e-6f)return std::abs(p)<=half;float a=(-half-p)/d,c=(half-p)/d;if(a>c)std::swap(a,c);lo=std::max(lo,a);hi=std::min(hi,c);return lo<=hi;};
        return slab(x,dx,b.w/2)&&slab(y,dy,b.h/2)&&slab(z,dz,b.d/2)?lo:limit;
    }
    bool remove() {
        // Select the closest player-built object along the horizontal sight ray.
        size_t chosen=objects.size();float nearest=9;
        const float fx=-std::sin(yaw)*std::cos(pitch),fy=std::sin(pitch),fz=-std::cos(yaw)*std::cos(pitch);
        for(size_t i=0;i<objects.size();++i) {
            float hit=rayDistance(objects[i],x,y,z,fx,fy,fz,nearest);
            if(hit<nearest){nearest=hit;chosen=i;}
        }
        if(chosen==objects.size())return false;
        objects.erase(objects.begin()+chosen);return true;
    }
    bool save(const std::string &path) const {
        const std::string temporary=path+".tmp";
        {std::ofstream f(temporary);if(!f)return false;f<<"EKRON_SANDBOX_6\n"<<objects.size()<<'\n';for(auto o:objects)f<<o.type<<' '<<o.rotation<<' '<<o.x<<' '<<o.z<<' '<<o.y<<' '<<o.anchored<<' '<<o.surface<<' '<<o.frozen<<' '<<o.tiltX<<' '<<o.tiltZ<<'\n';f.flush();if(!f)return false;}
        return std::rename(temporary.c_str(),path.c_str())==0;
    }
    bool load(const std::string &path) {
        std::ifstream f(path);std::string magic;int count;
        if(!(f>>magic>>count)||(magic!="EKRON_SANDBOX_1"&&magic!="EKRON_SANDBOX_2"&&magic!="EKRON_SANDBOX_3"&&magic!="EKRON_SANDBOX_4"&&magic!="EKRON_SANDBOX_5"&&magic!="EKRON_SANDBOX_6")||count<0||count>limit)return false;
        std::vector<Object> incoming;
        for(int i=0;i<count;i++){Object o{};if(!(f>>o.type>>o.rotation>>o.x>>o.z)||(magic!="EKRON_SANDBOX_1"&&!(f>>o.y))||((magic=="EKRON_SANDBOX_3"||magic=="EKRON_SANDBOX_4"||magic=="EKRON_SANDBOX_5"||magic=="EKRON_SANDBOX_6")&&!(f>>o.anchored))||((magic=="EKRON_SANDBOX_4"||magic=="EKRON_SANDBOX_5"||magic=="EKRON_SANDBOX_6")&&!(f>>o.surface))||((magic=="EKRON_SANDBOX_5"||magic=="EKRON_SANDBOX_6")&&!(f>>o.frozen))||(magic=="EKRON_SANDBOX_6"&&(!(f>>o.tiltX>>o.tiltZ)||!std::isfinite(o.tiltX)||!std::isfinite(o.tiltZ)))||o.surface < -1||o.surface>6||o.type<0||o.type>=kObjectTypeCount||o.rotation<0||o.rotation>=4||!std::isfinite(o.x)||!std::isfinite(o.z)||!std::isfinite(o.y)||std::abs(o.y)>200||std::abs(o.x)>62||std::abs(o.z)>62)return false;if(o.type==8)o.yaw=o.rotation*1.57079632679f;incoming.push_back(o);}
        objects=std::move(incoming);return true;
    }
private:
    void solid(Box b){walls.push_back(b);scenery.push_back(b);}
    void building(float bx,float bz,bool damaged) {
        const float h=damaged?7.5f:10.f;
        scenery.push_back({bx,.1f,bz,12,.2f,12,.44f,.44f,.39f});
        solid({bx-3.8f,1.5f,bz+6,4.4f,3,.35f,.53f,.51f,.45f});
        solid({bx+3.8f,1.5f,bz+6,4.4f,3,.35f,.53f,.51f,.45f});
        // A real 3.2 m wide doorway is left between the front wall segments.
        scenery.push_back({bx,3.3f,bz+6,12,.6f,.35f,.53f,.51f,.45f});
        solid({bx-6,h/2,bz,.4f,h,12,.55f,.54f,.49f});
        solid({bx+6,h/2,bz,.4f,h,12,.55f,.54f,.49f});
        solid({bx,h/2,bz-6,12,h,.4f,.55f,.54f,.49f});
        if(damaged) scenery.push_back({bx-4,5.4f,bz+6,4,3.6f,.35f,.5f,.48f,.42f});
        else {
            scenery.push_back({bx,6.7f,bz+6,12,6.2f,.35f,.58f,.58f,.53f});
            scenery.push_back({bx,10.1f,bz,12.5f,.2f,12.5f,.34f,.36f,.33f});
        }
        for(int floor=0;floor<3;floor++)for(float offset:{-4.f,0.f,4.f}) {
            if((floor==0&&offset==0)||(damaged&&floor>0&&offset>=0))continue;
            scenery.push_back({bx+offset,1.8f+floor*2.9f,bz+6.2f,1.7f,1.6f,.07f,.08f,.1f,.11f,Plain});
        }
        solid({bx-2,.45f,bz-1,1.8f,.9f,1,.5f,.37f,.26f});
        solid({bx+4,1.05f,bz-4,1.4f,2.1f,.65f,.4f,.3f,.22f});
        if(damaged)for(int i=0;i<8;i++)solid({bx+(i%2?1.f:-1.f)*(3.f+i%3),.2f,bz+7+(i*3%4),.8f,.4f,.6f,1.f,1.f,1.f,Brick});
    }
};
}
