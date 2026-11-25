#pragma once
#include "SandboxWorld.h"
#include <vector>

namespace sandbox {
struct MeshVertex {
    float x,y,z,r,g,b,nx,ny,nz,u,v,layer;
};
inline void appendCube(std::vector<MeshVertex>&vertices,const Box&box) {
    static constexpr int faces[6][6]={{0,1,2,0,2,3},{5,4,7,5,7,6},{4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
    static constexpr float corners[8][3]={{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1},{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1}};
    static constexpr float normals[6][3]={{0,0,1},{0,0,-1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0}};
    // One repeat every 2 world metres, independent of wall size.
    constexpr float repeatPerMetre=.5f;
    for(int face=0;face<6;face++)for(int index:faces[face]) {
        const auto *c=corners[index],*n=normals[face];
        float x=box.x+c[0]*box.w/2,y=box.y+c[1]*box.h/2,z=box.z+c[2]*box.d/2;
        float u=face<2?x:face<4?z:x;
        float v=face<4?y:z;
        // Textured albedo is already colored; a stronger tint lets scene props
        // (green dumpster, red barrel, orange cone) keep their designed hue.
        const bool plain=box.material==Plain;
        float r=plain?box.r:.45f+box.r*.55f,g=plain?box.g:.45f+box.g*.55f,b=plain?box.b:.45f+box.b*.55f;
        vertices.push_back({x,y,z,r,g,b,n[0],n[1],n[2],u*repeatPerMetre,v*repeatPerMetre,static_cast<float>(box.material)});
    }
}
// Vertical frustum (a cylinder when rTop==rBottom, a cone when rTop==0) centred
// at (cX,cY,cZ). Rotationally symmetric about Y, so the catalogue yaw needs no
// extra transform from the caller.
inline void appendFrustum(std::vector<MeshVertex>&v,float cX,float cY,float cZ,float rBottom,float rTop,float halfY,float r,float g,float b,int material,int sides=20){
    const bool plain=material==Plain;
    float tr=plain?r:.45f+r*.55f,tg=plain?g:.45f+g*.55f,tb=plain?b:.45f+b*.55f;
    auto add=[&](float x,float y,float z,float nx,float ny,float nz,float u,float w){v.push_back({cX+x,cY+y,cZ+z,tr,tg,tb,nx,ny,nz,u,w,float(material)});};
    float slope=(rBottom-rTop)/(2*halfY),inv=1.f/std::sqrt(1+slope*slope);
    for(int i=0;i<sides;i++){
        float a0=6.2831853f*i/sides,a1=6.2831853f*(i+1)/sides;
        float x0=std::cos(a0),z0=std::sin(a0),x1=std::cos(a1),z1=std::sin(a1);
        add(x0*rBottom,-halfY,z0*rBottom,x0*inv,slope*inv,z0*inv,float(i)/sides,0.f);
        add(x1*rBottom,-halfY,z1*rBottom,x1*inv,slope*inv,z1*inv,float(i+1)/sides,0.f);
        add(x1*rTop, halfY,z1*rTop   ,x1*inv,slope*inv,z1*inv,float(i+1)/sides,1.f);
        add(x0*rBottom,-halfY,z0*rBottom,x0*inv,slope*inv,z0*inv,float(i)/sides,0.f);
        add(x1*rTop, halfY,z1*rTop   ,x1*inv,slope*inv,z1*inv,float(i+1)/sides,1.f);
        add(x0*rTop, halfY,z0*rTop   ,x0*inv,slope*inv,z0*inv,float(i)/sides,1.f);
    }
    if(rBottom>0)for(int i=0;i<sides;i++){
        float a0=6.2831853f*i/sides,a1=6.2831853f*(i+1)/sides;
        float x0=std::cos(a0)*rBottom,z0=std::sin(a0)*rBottom,x1=std::cos(a1)*rBottom,z1=std::sin(a1)*rBottom;
        add(0,-halfY,0,0,-1,0,.5f,.5f);
        add(x1,-halfY,z1,0,-1,0,(std::cos(a1)+1)*.5f,(std::sin(a1)+1)*.5f);
        add(x0,-halfY,z0,0,-1,0,(std::cos(a0)+1)*.5f,(std::sin(a0)+1)*.5f);
    }
    if(rTop>0)for(int i=0;i<sides;i++){
        float a0=6.2831853f*i/sides,a1=6.2831853f*(i+1)/sides;
        float x0=std::cos(a0)*rTop,z0=std::sin(a0)*rTop,x1=std::cos(a1)*rTop,z1=std::sin(a1)*rTop;
        add(0,halfY,0,0,1,0,.5f,.5f);
        add(x0,halfY,z0,0,1,0,(std::cos(a0)+1)*.5f,(std::sin(a0)+1)*.5f);
        add(x1,halfY,z1,0,1,0,(std::cos(a1)+1)*.5f,(std::sin(a1)+1)*.5f);
    }
}
inline void appendCylinder(std::vector<MeshVertex>&v,float cX,float cY,float cZ,float radius,float halfY,float r,float g,float b,int material,int sides=20){appendFrustum(v,cX,cY,cZ,radius,radius,halfY,r,g,b,material,sides);}
inline void appendCone(std::vector<MeshVertex>&v,float cX,float cY,float cZ,float baseRadius,float halfY,float r,float g,float b,int material,int sides=20){appendFrustum(v,cX,cY,cZ,baseRadius,0,halfY,r,g,b,material,sides);}
inline void appendSphere(std::vector<MeshVertex>&v,float cX,float cY,float cZ,float radius,float r,float g,float b,int material=Plain,int rings=7,int sides=12){
    const bool plain=material==Plain;
    float tr=plain?r:.45f+r*.55f,tg=plain?g:.45f+g*.55f,tb=plain?b:.45f+b*.55f;
    for(int ring=0;ring<rings;ring++)for(int side=0;side<sides;side++){
        auto V=[&](int rr,int ss){float a=3.14159265f*rr/rings,b=6.2831853f*ss/sides;float nx=std::sin(a)*std::cos(b),ny=std::cos(a),nz=std::sin(a)*std::sin(b);return std::array<float,6>{nx*radius,ny*radius,nz*radius,nx,ny,nz};};
        auto A=V(ring,side),B=V(ring+1,side),C=V(ring+1,side+1),D=V(ring,side+1);
        v.push_back({cX+A[0],cY+A[1],cZ+A[2],tr,tg,tb,A[3],A[4],A[5],0,0,float(material)});
        v.push_back({cX+B[0],cY+B[1],cZ+B[2],tr,tg,tb,B[3],B[4],B[5],0,0,float(material)});
        v.push_back({cX+C[0],cY+C[1],cZ+C[2],tr,tg,tb,C[3],C[4],C[5],0,0,float(material)});
        v.push_back({cX+A[0],cY+A[1],cZ+A[2],tr,tg,tb,A[3],A[4],A[5],0,0,float(material)});
        v.push_back({cX+C[0],cY+C[1],cZ+C[2],tr,tg,tb,C[3],C[4],C[5],0,0,float(material)});
        v.push_back({cX+D[0],cY+D[1],cZ+D[2],tr,tg,tb,D[3],D[4],D[5],0,0,float(material)});
    }
}
}
