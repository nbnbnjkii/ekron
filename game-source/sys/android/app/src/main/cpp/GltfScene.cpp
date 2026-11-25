#include "GltfScene.h"
#include "SceneCollision.h"
#include "MaterialRules.h"
#include "LevelPlacement.h"
#include "BuildingCollision.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_FS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "third_party/tiny_gltf.h"
#pragma GCC diagnostic pop
#include <cstring>
#include <limits>
#include <stdexcept>
#include <set>
#include <optional>

namespace city {
namespace {
using Vec=std::array<float,3>;
using Matrix=std::array<double,16>;
Matrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
Matrix multiply(const Matrix&a,const Matrix&b){Matrix r{};for(int c=0;c<4;c++)for(int row=0;row<4;row++)for(int k=0;k<4;k++)r[c*4+row]+=a[k*4+row]*b[c*4+k];return r;}
Vec point(const Matrix&m,Vec v){return {float(m[0]*v[0]+m[4]*v[1]+m[8]*v[2]+m[12]),float(m[1]*v[0]+m[5]*v[1]+m[9]*v[2]+m[13]),float(m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14])};}
Vec cross(Vec a,Vec b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
Vec normal(Vec a){float l=std::sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);return l>.000001f?Vec{a[0]/l,a[1]/l,a[2]/l}:Vec{0,1,0};}
Matrix transform(const tinygltf::Node&n){
 if(n.matrix.size()==16){Matrix m;std::copy(n.matrix.begin(),n.matrix.end(),m.begin());return m;}
 Matrix t=identity(),r=identity(),s=identity();
 if(n.translation.size()==3)for(int i=0;i<3;i++)t[12+i]=n.translation[i];
 if(n.scale.size()==3)for(int i=0;i<3;i++)s[i*5]=n.scale[i];
 if(n.rotation.size()==4){double x=n.rotation[0],y=n.rotation[1],z=n.rotation[2],w=n.rotation[3];double l=std::sqrt(x*x+y*y+z*z+w*w);if(l>0){x/=l;y/=l;z/=l;w/=l;}r={1-2*y*y-2*z*z,2*x*y+2*z*w,2*x*z-2*y*w,0,2*x*y-2*z*w,1-2*x*x-2*z*z,2*y*z+2*x*w,0,2*x*z+2*y*w,2*y*z-2*x*w,1-2*x*x-2*y*y,0,0,0,0,1};}
 return multiply(t,multiply(r,s));
}
// Normal inverse-transpose handles nonuniform scale and reflected nodes.
Vec transformedNormal(const Matrix&m,Vec n){
 Vec a{float(m[0]),float(m[1]),float(m[2])},b{float(m[4]),float(m[5]),float(m[6])},c{float(m[8]),float(m[9]),float(m[10])};
 auto u=cross(b,c),v=cross(c,a),w=cross(a,b);float det=a[0]*u[0]+a[1]*u[1]+a[2]*u[2];
 if(std::abs(det)<1e-12f)return {0,1,0};
 return normal({(u[0]*n[0]+v[0]*n[1]+w[0]*n[2])/det,(u[1]*n[0]+v[1]*n[1]+w[1]*n[2])/det,(u[2]*n[0]+v[2]*n[1]+w[2]*n[2])/det});
}
struct Access {
 const tinygltf::Accessor &a;const std::vector<unsigned char>&data;size_t offset,stride,bytes;int channels;
 Access(const tinygltf::Model&m,int index):a(m.accessors.at(index)),data(m.buffers.at(m.bufferViews.at(a.bufferView).buffer).data){
  if(a.sparse.isSparse)throw std::runtime_error("Sparse accessors are not supported");
  const auto &view=m.bufferViews.at(a.bufferView);
  channels=tinygltf::GetNumComponentsInType(a.type);int component=tinygltf::GetComponentSizeInBytes(a.componentType);
  if(channels<=0||component<=0)throw std::runtime_error("Invalid accessor type");
  bytes=size_t(channels)*component;int step=a.ByteStride(view);if(step<0)throw std::runtime_error("Invalid accessor stride");stride=size_t(step);offset=view.byteOffset+a.byteOffset;
  if(stride<bytes||view.byteOffset>data.size()||view.byteLength>data.size()-view.byteOffset||a.byteOffset>view.byteLength||offset>data.size()|| (a.count && (bytes>view.byteLength-a.byteOffset||(a.count-1)>(view.byteLength-a.byteOffset-bytes)/stride)))throw std::runtime_error("Accessor outside buffer");
 }
 float get(size_t i,int c)const{
  if(i>=a.count||c>=channels)throw std::runtime_error("Accessor index out of range");
  const auto*p=data.data()+offset+i*stride+c*(bytes/channels);
  switch(a.componentType){case TINYGLTF_COMPONENT_TYPE_FLOAT:{float v;std::memcpy(&v,p,4);if(!std::isfinite(v))throw std::runtime_error("Non-finite vertex");return v;}
   case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:return a.normalized?*p/255.f:*p;
   case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:{uint16_t v;std::memcpy(&v,p,2);return a.normalized?v/65535.f:v;}
   case TINYGLTF_COMPONENT_TYPE_BYTE:{int8_t v;std::memcpy(&v,p,1);return a.normalized?std::max(-1.f,v/127.f):v;}
   case TINYGLTF_COMPONENT_TYPE_SHORT:{int16_t v;std::memcpy(&v,p,2);return a.normalized?std::max(-1.f,v/32767.f):v;}
   default:throw std::runtime_error("Unsupported vertex component");}
 }
 size_t index(size_t i)const{
  if(channels!=1||i>=a.count)throw std::runtime_error("Invalid index accessor");
  auto*p=data.data()+offset+i*stride;
  if(a.componentType==5121)return *p;
  if(a.componentType==5123){uint16_t v;std::memcpy(&v,p,2);return v;}
  if(a.componentType==5125){uint32_t v;std::memcpy(&v,p,4);return v;}throw std::runtime_error("Invalid index type");
 }
};
struct DecodePolicy {bool selective=false;std::set<int> required;};
bool imageLoader(tinygltf::Image *image,int index,std::string*,std::string *warn,int,int,const unsigned char *bytes,int size,void*user){
 auto *policy=static_cast<DecodePolicy*>(user);
 if(policy&&policy->selective&&index>=0&&!policy->required.count(index)){
  image->width=image->height=1;image->component=4;image->bits=8;image->pixel_type=5121;image->image={255,255,255,255};return true;
 }
 int w=0,h=0,c=0;unsigned char*p=nullptr;
 if(size>0&&stbi_info_from_memory(bytes,size,&w,&h,&c)&&w>0&&h>0&&w<=8192&&h<=8192)p=stbi_load_from_memory(bytes,size,&w,&h,&c,4);
 if(!p){image->width=image->height=1;image->component=4;image->bits=8;image->pixel_type=5121;image->image={255,255,255,255};if(warn)*warn+="Image decode failed; white fallback used.\n";return true;}
 int rw=std::min(w,1024),rh=std::min(h,1024);image->width=rw;image->height=rh;image->component=4;image->bits=8;image->pixel_type=5121;image->image.resize(size_t(rw)*rh*4);
 for(int y=0;y<rh;y++)for(int x=0;x<rw;x++)std::copy_n(p+(size_t(y*h/rh)*w+x*w/rw)*4,4,image->image.data()+(size_t(y)*rw+x)*4);
 stbi_image_free(p);return true;
}
void applyFallbackUV(Scene&s){
 for(auto &batch:s.batches)if(batch.material>=0&&s.materials[batch.material].worldSpaceUV)for(auto &v:batch.vertices){
  if(std::abs(v.ny)>=std::max(std::abs(v.nx),std::abs(v.nz))){v.u=v.x*.5f;v.v=v.z*.5f;}
  else if(std::abs(v.nx)>std::abs(v.nz)){v.u=v.z*.5f;v.v=v.y*.5f;}
  else{v.u=v.x*.5f;v.v=v.y*.5f;}
 }
}
void prepare(Scene &s,const LoadOptions &options){
 if(options.progress)options.progress(55);
 Vec lo{1e20f,1e20f,1e20f},hi{-1e20f,-1e20f,-1e20f};size_t count=0;
 for(auto &b:s.batches)for(auto &v:b.vertices){Vec p{v.x,v.y,v.z};for(int i=0;i<3;i++){lo[i]=std::min(lo[i],p[i]);hi[i]=std::max(hi[i],p[i]);}count++;}
 if(!count)throw std::runtime_error("No triangle geometry");
 const float extent=std::max(hi[0]-lo[0],hi[2]-lo[2]);if(extent<.01f)throw std::runtime_error("Degenerate scene bounds");
 const float scale=options.targetExtent/extent,cx=(lo[0]+hi[0])/2,cz=(lo[2]+hi[2])/2;
 if(!options.level){
  float cosine=std::cos(options.yaw),sine=std::sin(options.yaw);
  Vec minimum{1e20f,1e20f,1e20f},maximum{-1e20f,-1e20f,-1e20f};
  for(auto &batch:s.batches)for(auto &v:batch.vertices){
   float x=(v.x-cx)*scale,z=(v.z-cz)*scale;
   v.x=options.x+cosine*x+sine*z;v.z=options.z-sine*x+cosine*z;v.y=options.y+(v.y-lo[1])*scale;
   float nx=v.nx,nz=v.nz;v.nx=cosine*nx+sine*nz;v.nz=-sine*nx+cosine*nz;
   Vec p{v.x,v.y,v.z};for(int i=0;i<3;i++){minimum[i]=std::min(minimum[i],p[i]);maximum[i]=std::max(maximum[i],p[i]);}
  }
  s.minimum=minimum;s.maximum=maximum;
  float height=std::min(1.2f,maximum[1]-minimum[1]);
  s.colliders.push_back({(minimum[0]+maximum[0])/2,minimum[1]+height/2,(minimum[2]+maximum[2])/2,maximum[0]-minimum[0],height,maximum[2]-minimum[2],0,0,0});
  applyFallbackUV(s);
  if(options.buildPhysics){buildSceneCollision(s);}
  return;
 }
 // Estimate ground from area-weighted horizontal triangles, rather than lowest rubble vertex.
 std::map<int,double> ground;
 for(auto &b:s.batches)for(size_t i=0;i<b.vertices.size();i+=3){const auto&a=b.vertices[i],&v=b.vertices[i+1],&c=b.vertices[i+2];auto n=cross({v.x-a.x,v.y-a.y,v.z-a.z},{c.x-a.x,c.y-a.y,c.z-a.z});if(std::abs(normal(n)[1])>.9f)ground[int(std::round((a.y+v.y+c.y)/3*scale*10))]+=std::abs(n[1]);}
 float floor=lo[1];double weight=0;for(auto [key,value]:ground)if(value>weight){weight=value;floor=key/(10*scale);}
 for(auto &b:s.batches)for(auto &v:b.vertices){v.x=(v.x-cx)*scale;v.z=(v.z-cz)*scale;v.y=(v.y-floor)*scale;}
 s.minimum={(lo[0]-cx)*scale,(lo[1]-floor)*scale,(lo[2]-cz)*scale};s.maximum={(hi[0]-cx)*scale,(hi[1]-floor)*scale,(hi[2]-cz)*scale};
 // Only physical building wall slices block walking. Ground is the existing
 // continuous y=0 walking plane: missing/uneven mesh coverage must NOT become a wall.
 for(const auto &batch:s.batches){
  bool opaque=batch.material<0||size_t(batch.material)>=s.materials.size()||s.materials[batch.material].alphaMode==0;
  for(size_t i=0;i<batch.vertices.size();i+=3){
   const auto &a=batch.vertices[i],&b=batch.vertices[i+1],&c=batch.vertices[i+2];
   addBuildingWall(s.wallSegments,{a.x,a.y,a.z},{b.x,b.y,b.z},{c.x,c.y,c.z},opaque);
  }
 }
 // Thin outer perimeter only; never fill indoor areas or unsupported road cells.
 float minX=s.minimum[0]-1,maxX=s.maximum[0]+1,minZ=s.minimum[2]-1,maxZ=s.maximum[2]+1;
 s.colliders={{minX-.1f,1,(minZ+maxZ)/2,.2f,2,maxZ-minZ,0,0,0},
              {maxX+.1f,1,(minZ+maxZ)/2,.2f,2,maxZ-minZ,0,0,0},
              {(minX+maxX)/2,1,minZ-.1f,maxX-minX,.2f,.2f,0,0,0},
              {(minX+maxX)/2,1,maxZ+.1f,maxX-minX,.2f,.2f,0,0,0}};
 if(options.progress)options.progress(65);
 sandbox::World collisionWorld;collisionWorld.walls=s.colliders;collisionWorld.wallSegments=s.wallSegments;
 SpawnPoint spawn;
 if(!findRoadSpawn(collisionWorld,s.minimum,s.maximum,options.preferredSpawnX,options.preferredSpawnZ,spawn))throw std::runtime_error("No connected, supported free spawn in the collision grid");
 s.spawnX=spawn.x;s.spawnZ=spawn.z;
 applyFallbackUV(s);
 if(options.progress)options.progress(80);
 buildSceneCollision(s);
 if(options.progress)options.progress(100);
}
}

bool Scene::load(const std::string &path,const ReadFile &read,std::string &diagnostics,const LoadOptions &options){
 try{
  Scene result;std::vector<unsigned char> bytes;if(!read(path,bytes)||bytes.empty()||bytes.size()>128*1024*1024)throw std::runtime_error("Model missing or exceeds 128 MiB");
  if(options.progress)options.progress(5);
  DecodePolicy policy;
  try{
   size_t offset=0,length=bytes.size();
   if(bytes.size()>20&&std::memcmp(bytes.data(),"glTF",4)==0){uint32_t n=0;std::memcpy(&n,bytes.data()+12,4);offset=20;length=n;}
   if(offset+length<=bytes.size()){
    auto j=nlohmann::json::parse(bytes.begin()+offset,bytes.begin()+offset+length);
    if(j.contains("materials")&&j.contains("textures")){
     for(const auto&m:j["materials"])if(m.contains("pbrMetallicRoughness")&&m["pbrMetallicRoughness"].contains("baseColorTexture")){
      int texture=m["pbrMetallicRoughness"]["baseColorTexture"]["index"].get<int>();
      const auto&t=j["textures"].at(texture);if(t.contains("source"))policy.required.insert(t["source"].get<int>());
     }
     policy.selective=true;
    }
   }
  }catch(const std::exception&){policy.selective=false;}
  tinygltf::TinyGLTF parser;parser.SetImageLoader(imageLoader,&policy);
  tinygltf::FsCallbacks fs;
  fs.FileExists=[&](const std::string&p,void*){std::vector<unsigned char>b;return read(p,b);};
  fs.ExpandFilePath=[](const std::string&p,void*){return p;};
  fs.ReadWholeFile=[&](std::vector<unsigned char>*b,std::string*,const std::string&p,void*){return read(p,*b);};
  fs.WriteWholeFile=[](std::string*,const std::string&,const std::vector<unsigned char>&,void*){return false;};
  fs.GetFileSizeInBytes=[&](size_t*size,std::string*,const std::string&p,void*){std::vector<unsigned char>b;if(!read(p,b))return false;*size=b.size();return true;};fs.user_data=nullptr;parser.SetFsCallbacks(fs);
  tinygltf::Model model;std::string error,warn;const auto slash=path.find_last_of('/');const std::string base=slash==std::string::npos?"":path.substr(0,slash+1);
  bool binary=bytes.size()>=4&&std::memcmp(bytes.data(),"glTF",4)==0;
  bool ok=binary?parser.LoadBinaryFromMemory(&model,&error,&warn,bytes.data(),static_cast<unsigned>(bytes.size()),base):parser.LoadASCIIFromString(&model,&error,&warn,reinterpret_cast<const char*>(bytes.data()),static_cast<unsigned>(bytes.size()),base);
  if(options.progress)options.progress(20);
  diagnostics=warn;if(!ok)throw std::runtime_error(error);
  if(!model.extensionsRequired.empty())throw std::runtime_error("Required glTF extensions are not supported by this static renderer");
  for(auto &image:model.images)result.images.push_back({image.width,image.height,std::move(image.image)});
  for(const auto&m:model.materials){Material out;out.playerFence=options.level&&m.name=="Material.006";auto&p=m.pbrMetallicRoughness;out.roughness=float(p.roughnessFactor);for(int i=0;i<4;i++)out.color[i]=float(p.baseColorFactor.at(i));out.doubleSided=m.doubleSided;out.alphaMode=m.alphaMode=="BLEND"?2:m.alphaMode=="MASK"?1:0;out.cutoff=float(m.alphaCutoff);
   if(p.baseColorTexture.index>=0){if(p.baseColorTexture.texCoord!=0)throw std::runtime_error("Only TEXCOORD_0 base color is supported");const auto&tex=model.textures.at(p.baseColorTexture.index);out.image=tex.source;if(tex.sampler>=0){auto &sampler=model.samplers.at(tex.sampler);out.wrapS=sampler.wrapS;out.wrapT=sampler.wrapT;}}
   std::vector<unsigned char> overrideBytes;
   if(read(base+"overrides/material_"+std::to_string(result.materials.size())+".png",overrideBytes)&&!overrideBytes.empty()&&overrideBytes.size()<=32*1024*1024){
    tinygltf::Image image;imageLoader(&image,0,nullptr,&diagnostics,0,0,overrideBytes.data(),static_cast<int>(overrideBytes.size()),nullptr);
    out.image=static_cast<int>(result.images.size());result.images.push_back({image.width,image.height,std::move(image.image)});
   }
   result.materials.push_back(out);
  }
  std::map<std::string,int> fallbackImages;
  std::set<int> stack;size_t vertexCount=0;
  std::function<void(int,const Matrix&)> visit=[&](int index,const Matrix &parent){
   if(stack.size()>128||!stack.insert(index).second)throw std::runtime_error("Cyclic or excessively deep nodes");
   const auto &node=model.nodes.at(index);auto matrix=multiply(parent,transform(node));
   if(node.skin>=0)throw std::runtime_error("Skinned models are not supported");
   if(node.mesh>=0)for(const auto &p:model.meshes.at(node.mesh).primitives){
    if(p.mode!=-1&&p.mode!=TINYGLTF_MODE_TRIANGLES)throw std::runtime_error("Only triangle primitives are supported");
    Access positions(model,p.attributes.at("POSITION"));if(positions.channels!=3)throw std::runtime_error("POSITION must be VEC3");
    std::optional<Access> normals,uv,indices,colors;if(p.attributes.count("COLOR_0"))colors.emplace(model,p.attributes.at("COLOR_0"));if(p.attributes.count("NORMAL"))normals.emplace(model,p.attributes.at("NORMAL"));if(p.attributes.count("TEXCOORD_0"))uv.emplace(model,p.attributes.at("TEXCOORD_0"));if(p.indices>=0)indices.emplace(model,p.indices);
    size_t count=indices?indices->a.count:positions.a.count;if(count%3||vertexCount+count>6000000)throw std::runtime_error("Invalid or excessive triangle count");vertexCount+=count;
    Batch batch;batch.rotor=options.animateRotors&&(index==16||index==18||index==20||index==22);
    if(options.level)for(const char*wire:{"Plane.009_","Plane.010_","Plane.011_","Plane.012_","Plane.013_","Plane.014_","Plane.015_"})if(node.name.rfind(wire,0)==0)batch.decorativeWire=true;
    batch.material=p.material;batch.vertices.reserve(count);
    Material original=batch.material>=0?result.materials.at(batch.material):Material{};
    auto style=sandbox::materialForName(node.name+" "+model.meshes.at(node.mesh).name);
    if(original.image<0&&style.layer!=sandbox::Plain){
     original.roughness=style.roughness;original.worldSpaceUV=true;original.wrapS=original.wrapT=10497;std::string key=style.texture;
     if(!fallbackImages.count(key)){
      int imageIndex=-1;std::vector<unsigned char> texture;
      if(read("textures/"+key+".png",texture)&&!texture.empty()&&texture.size()<=32*1024*1024){
       tinygltf::Image image;imageLoader(&image,-1,nullptr,&diagnostics,0,0,texture.data(),int(texture.size()),nullptr);
       imageIndex=int(result.images.size());result.images.push_back({image.width,image.height,std::move(image.image)});
      }fallbackImages[key]=imageIndex;
     }
     original.image=fallbackImages[key];
     for(int component=0;component<3;component++)original.color[component]=original.image>=0?1:style.color[component];
     batch.material=int(result.materials.size());result.materials.push_back(original);
    }

    for(size_t i=0;i<count;i++){size_t idx=indices?indices->index(i):i;Vec v=point(matrix,{positions.get(idx,0),positions.get(idx,1),positions.get(idx,2)}),n=normals?transformedNormal(matrix,{normals->get(idx,0),normals->get(idx,1),normals->get(idx,2)}):Vec{0,1,0};batch.vertices.push_back({v[0],v[1],v[2],n[0],n[1],n[2],uv?uv->get(idx,0):0,uv?uv->get(idx,1):0,colors?colors->get(idx,0):1,colors?colors->get(idx,1):1,colors?colors->get(idx,2):1});}
    if(!normals)for(size_t i=0;i<count;i+=3){auto &a=batch.vertices[i],&b=batch.vertices[i+1],&c=batch.vertices[i+2];auto n=normal(cross({b.x-a.x,b.y-a.y,b.z-a.z},{c.x-a.x,c.y-a.y,c.z-a.z}));for(size_t k=i;k<i+3;k++){batch.vertices[k].nx=n[0];batch.vertices[k].ny=n[1];batch.vertices[k].nz=n[2];}}
    const double det=matrix[0]*(matrix[5]*matrix[10]-matrix[9]*matrix[6])-matrix[4]*(matrix[1]*matrix[10]-matrix[9]*matrix[2])+matrix[8]*(matrix[1]*matrix[6]-matrix[5]*matrix[2]);
    if(det<0)for(size_t i=0;i<count;i+=3)std::swap(batch.vertices[i+1],batch.vertices[i+2]);
    result.batches.push_back(std::move(batch));
   }
   for(int child:node.children)visit(child,matrix);
   stack.erase(index);
  };
  if(model.scenes.empty())throw std::runtime_error("No glTF scene");
  int scene=model.defaultScene>=0?model.defaultScene:0;for(int node:model.scenes.at(scene).nodes)visit(node,identity());
  if(!std::isfinite(options.targetExtent)||options.targetExtent<=0||!std::isfinite(options.x)||!std::isfinite(options.z)||!std::isfinite(options.y)||!std::isfinite(options.yaw))throw std::runtime_error("Invalid scene placement options");
  if(options.progress)options.progress(50);
  prepare(result,options);*this=std::move(result);diagnostics+="Loaded "+std::to_string(vertexCount/3)+" triangles, "+std::to_string(materials.size())+" materials, "+std::to_string(colliders.size())+" boxes, "+std::to_string(wallSegments.size())+" thin wall segments.\n";return true;
 }catch(const std::exception&e){diagnostics+="glTF load failed: "+std::string(e.what());return false;}
}
}
