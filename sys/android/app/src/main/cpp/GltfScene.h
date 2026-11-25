#pragma once
#include <array>
#include <functional>
#include <string>
#include <vector>
#include "SandboxWorld.h"
#include "CollisionMesh.h"
#include <memory>

namespace city {
using ReadFile=std::function<bool(const std::string&,std::vector<unsigned char>&)>;
struct Vertex { float x,y,z,nx,ny,nz,u,v; float r=1,g=1,b=1; };
struct Image { int width=0,height=0;std::vector<unsigned char> rgba; };
struct Material {
    std::array<float,4> color{1,1,1,1};
    int image=-1,wrapS=10497,wrapT=10497;
    bool doubleSided=false;
    bool playerFence=false; // Explicit city-asset fence material, never inferred from transparency alone.
    int alphaMode=0; // 0 opaque, 1 mask, 2 blend
    float cutoff=.5f;
    float roughness=.9f;
    bool worldSpaceUV=false;
};
// Identical source image + wrap modes can share GPU storage. Material color,
// roughness and transparency remain per draw, not part of the texture object.
inline size_t sharedTextureOwner(const std::vector<Material>&materials,size_t index){
 const auto&m=materials.at(index);
 for(size_t j=0;j<index;j++){const auto&p=materials[j];if(p.image==m.image&&p.wrapS==m.wrapS&&p.wrapT==m.wrapT)return j;}
 return index;
}
struct Batch { std::vector<Vertex> vertices;int material=-1;bool decorativeWire=false;bool rotor=false; };
struct LoadOptions {
    // CPU loading milestones; optional, called on the loading worker.
    std::function<void(int)> progress;
    bool animateRotors=false;
    bool level=true;
    bool buildPhysics=true;
    float targetExtent=80.f;
    float preferredSpawnX=-3.4f,preferredSpawnZ=32.7f;
    float x=0,y=0,z=0,yaw=0;
};
struct Scene {
    std::vector<Image> images;
    std::vector<Material> materials;
    std::vector<Batch> batches;
    std::vector<sandbox::Box> colliders;
    std::vector<sandbox::WallSegment> wallSegments;
    std::shared_ptr<const physics::CollisionMesh> collision;
    std::shared_ptr<const physics::CollisionMesh> fenceBarriers;
    float spawnX=0,spawnZ=0;
    std::array<float,3> minimum{},maximum{};
    // Reads both GLB and JSON glTF from assets; external paths resolve relative to the scene.
    bool load(const std::string &path,const ReadFile &read,std::string &diagnostics,const LoadOptions &options={});
};
}
