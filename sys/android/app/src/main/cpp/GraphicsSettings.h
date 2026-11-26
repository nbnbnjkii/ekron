#pragma once
#include <algorithm>
namespace sandbox {
struct GraphicsSettings {
 int quality=1,fps=60;
 float scale()const{return quality<=0?.60f:quality==1?.80f:1.f;}
 int frameRate()const{return fps==30?30:60;}
 int dimension(int pixels)const{return std::max(1,int(std::max(1,pixels)*scale()));}
};
}
