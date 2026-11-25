#include "GltfRenderer.h"
#include <android/asset_manager.h>
#include <cstddef>
#include <numeric>

namespace city {
namespace {
bool readAsset(AAssetManager *manager,const std::string &path,std::vector<unsigned char>&bytes){
    // Never resolve URLs or escape the APK asset namespace.
    if(path.empty()||path[0]=='/'||path.find("..")!=std::string::npos||path.find(':')!=std::string::npos)return false;
    auto *asset=AAssetManager_open(manager,path.c_str(),AASSET_MODE_BUFFER);if(!asset)return false;
    auto size=AAsset_getLength64(asset);if(size<1||size>128*1024*1024){AAsset_close(asset);return false;}
    bytes.resize(static_cast<size_t>(size));size_t offset=0;
    while(offset<bytes.size()){int n=AAsset_read(asset,bytes.data()+offset,bytes.size()-offset);if(n<=0)break;offset+=static_cast<size_t>(n);}
    AAsset_close(asset);if(offset!=bytes.size()){bytes.clear();return false;}return true;
}
GLint wrap(int mode){return mode==GL_CLAMP_TO_EDGE||mode==GL_MIRRORED_REPEAT?mode:GL_REPEAT;}
}
bool GltfRenderer::prepare(AAssetManager *assets,const std::string&path,Scene &scene,std::string &log,const LoadOptions &options){
 return scene.load(path,[&](const std::string&p,std::vector<unsigned char>&b){return readAsset(assets,p,b);},log,options);
}
void GltfRenderer::beginUpload(Scene scene){release();scene_=std::move(scene);nextMaterial_=nextBatch_=0;uploading_=true;}
bool GltfRenderer::load(AAssetManager *assets,const std::string&path,std::string &log,const LoadOptions &options){
 Scene scene;if(!prepare(assets,path,scene,log,options))return false;beginUpload(std::move(scene));while(uploading_)uploadStep(log);return ready();
}
bool GltfRenderer::uploadStep(std::string &log){
    if(!uploading_)return ready();
    glActiveTexture(GL_TEXTURE1);glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    // Share identical image/sampler storage, while retaining material draw state.
    if(nextMaterial_<scene_.materials.size()){
        const size_t index=nextMaterial_++;
        const auto&material=scene_.materials[index];
        const size_t owner=sharedTextureOwner(scene_.materials,index);
        if(owner<index){textures_.push_back(textures_.at(owner));}else{
        GLuint texture=0;glGenTextures(1,&texture);textures_.push_back(texture);glBindTexture(GL_TEXTURE_2D,texture);
        const unsigned char white[4]={255,255,255,255};const Image *image=nullptr;
        if(material.image>=0&&size_t(material.image)<scene_.images.size())image=&scene_.images[material.image];
        const bool valid=image&&image->width>0&&image->height>0&&image->rgba.size()==size_t(image->width)*image->height*4;
        glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,valid?image->width:1,valid?image->height:1,0,GL_RGBA,GL_UNSIGNED_BYTE,valid?image->rgba.data():white);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrap(material.wrapS));glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrap(material.wrapT));
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glGenerateMipmap(GL_TEXTURE_2D);
        }
    }else if(nextBatch_<scene_.batches.size()){
        const auto&batch=scene_.batches[nextBatch_++];
        if(batch.vertices.empty())return false;Mesh mesh;mesh.count=static_cast<GLsizei>(batch.vertices.size());mesh.material=batch.material;
        for(const auto&v:batch.vertices){mesh.x+=v.x/mesh.count;mesh.z+=v.z/mesh.count;}
        mesh.rotor=batch.rotor;
        if(mesh.rotor){std::array<float,3> lo{1e9f,1e9f,1e9f},hi{-1e9f,-1e9f,-1e9f};for(const auto&v:batch.vertices){std::array<float,3> p{v.x,v.y,v.z};for(int k=0;k<3;k++){lo[k]=std::min(lo[k],p[k]);hi[k]=std::max(hi[k],p[k]);}}for(int k=0;k<3;k++)mesh.pivot[k]=(lo[k]+hi[k])*.5f;}
        glGenVertexArrays(1,&mesh.vao);glGenBuffers(1,&mesh.vbo);glBindVertexArray(mesh.vao);glBindBuffer(GL_ARRAY_BUFFER,mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER,batch.vertices.size()*sizeof(Vertex),batch.vertices.data(),GL_STATIC_DRAW);
        for(GLuint attr:{0u,1u,2u,3u})glEnableVertexAttribArray(attr);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,x)));
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,nx)));
        glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));
        meshes_.push_back(mesh);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);glActiveTexture(GL_TEXTURE0);
    auto error=glGetError();if(error!=GL_NO_ERROR){log+="glTF GPU upload failed: "+std::to_string(error);release();return false;}
    if(nextMaterial_<scene_.materials.size()||nextBatch_<scene_.batches.size())return false;
    uploading_=false;
    // Static VBOs/textures own the uploaded content; do not retain decoded images on CPU.
    for(auto&image:scene_.images)std::vector<unsigned char>().swap(image.rgba);
    for(auto&batch:scene_.batches)std::vector<Vertex>().swap(batch.vertices);
    return ready();
}
void GltfRenderer::draw(GLuint program,float eyeX,float eyeZ,bool hologram,float rotorAngle){
    if(!ready())return;
    glUniform1i(glGetUniformLocation(program,"modelPass"),1);glUniform1i(glGetUniformLocation(program,"modelAlbedo"),1);
    glActiveTexture(GL_TEXTURE1);glVertexAttrib3f(1,1,1,1);glVertexAttrib1f(4,4);
    std::vector<size_t> order(meshes_.size());std::iota(order.begin(),order.end(),0);
    auto blended=[&](const Mesh&m){return m.material>=0&&size_t(m.material)<scene_.materials.size()&&scene_.materials[m.material].alphaMode==2;};
    std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){const auto&ma=meshes_[a],&mb=meshes_[b];if(blended(ma)!=blended(mb))return !blended(ma);return blended(ma)&&std::hypot(ma.x-eyeX,ma.z-eyeZ)>std::hypot(mb.x-eyeX,mb.z-eyeZ);});
    for(size_t i:order){const auto&mesh=meshes_[i];Material fallback;const auto &material=mesh.material>=0&&size_t(mesh.material)<scene_.materials.size()?scene_.materials[mesh.material]:fallback;
        glUniform1f(glGetUniformLocation(program,"rotorAngle"),mesh.rotor?(mesh.pivot[0]*mesh.pivot[2]>0?-rotorAngle:rotorAngle):0);
        glUniform3fv(glGetUniformLocation(program,"rotorPivot"),1,mesh.pivot.data());
        glUniform1f(glGetUniformLocation(program,"modelRoughness"),material.roughness);
        glUniform4fv(glGetUniformLocation(program,"baseColorFactor"),1,material.color.data());glUniform1i(glGetUniformLocation(program,"modelTextured"),material.image>=0&&mesh.material>=0&&size_t(mesh.material)<textures_.size());
        glUniform1i(glGetUniformLocation(program,"alphaMode"),material.alphaMode);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),material.cutoff);
        if(mesh.material>=0&&size_t(mesh.material)<textures_.size())glBindTexture(GL_TEXTURE_2D,textures_[mesh.material]);
        if(hologram||material.alphaMode==2){glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(hologram?GL_TRUE:GL_FALSE);}else{glDisable(GL_BLEND);glDepthMask(GL_TRUE);}
        // Reflected/baked scene nodes and double-sided walls are rendered without culling.
        glDisable(GL_CULL_FACE);glBindVertexArray(mesh.vao);glDrawArrays(GL_TRIANGLES,0,mesh.count);
    }
    glDisable(GL_BLEND);glDepthMask(GL_TRUE);glActiveTexture(GL_TEXTURE0);glUniform1i(glGetUniformLocation(program,"modelPass"),0);
}
void GltfRenderer::release(){for(auto&m:meshes_){if(m.vbo)glDeleteBuffers(1,&m.vbo);if(m.vao)glDeleteVertexArrays(1,&m.vao);}meshes_.clear();if(!textures_.empty()){auto owned=textures_;std::sort(owned.begin(),owned.end());owned.erase(std::unique(owned.begin(),owned.end()),owned.end());glDeleteTextures(static_cast<GLsizei>(owned.size()),owned.data());}textures_.clear();scene_=Scene{};uploading_=false;}
}
