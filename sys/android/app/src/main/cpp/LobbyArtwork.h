#pragma once
#include <android/asset_manager.h>
#include "third_party/stb_image.h"
#include <future>
#include <vector>
namespace sandbox {
// Decode the baked original-design drawings off the UI/render threads.
inline std::vector<unsigned char> readLobbyArtwork(AAssetManager*manager){
 AAsset*asset=manager?AAssetManager_open(manager,"lobby/artwork.png",AASSET_MODE_BUFFER):nullptr;
 if(!asset)return {};
 auto size=AAsset_getLength64(asset);std::vector<unsigned char> compressed;
 if(size>0&&size<8*1024*1024){compressed.resize(size);size_t offset=0;
  while(offset<compressed.size()){int n=AAsset_read(asset,compressed.data()+offset,compressed.size()-offset);if(n<=0)break;offset+=n;}
  if(offset!=compressed.size())compressed.clear();
 }
 AAsset_close(asset);if(compressed.empty())return {};
 int w=0,h=0,channels=0;
 if(!stbi_info_from_memory(compressed.data(),int(compressed.size()),&w,&h,&channels)||w!=2048||h!=2048)return {};
 auto*rgba=stbi_load_from_memory(compressed.data(),int(compressed.size()),&w,&h,&channels,4);if(!rgba)return {};
 std::vector<unsigned char> result(rgba,rgba+w*h*4);stbi_image_free(rgba);return result;
}
}
