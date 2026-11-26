#include "../../sys/android/app/src/main/cpp/GraphicsSettings.h"
#include <cassert>
int main(){
 sandbox::GraphicsSettings medium{0,30},high{1,60},ultra{2,60};
 assert(medium.dimension(2000)==1200&&high.dimension(2000)==1600&&ultra.dimension(2000)==2000);
 assert(medium.frameRate()==30&&high.frameRate()==60);
 assert(medium.dimension(0)==1);sandbox::GraphicsSettings invalid{10,999};assert(invalid.frameRate()==60);
}
