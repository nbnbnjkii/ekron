#include "../../sys/android/app/src/main/cpp/SandboxWorld.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main() {
    sandbox::World world;
    assert(!world.scenery.empty());
    assert(world.place());
    assert(world.objects.size()==1);
    assert(!world.place()); // No intersecting duplicate placement.
    auto bounds=sandbox::World::bounds({0,1,0,0});
    assert(bounds.w==.25f && bounds.d==3.f);
    assert(!world.canPlace({sandbox::kObjectTypeCount,0,0,0}));
    assert(!world.canPlace({0,0,std::numeric_limits<float>::infinity(),0}));
    const std::string path="/tmp/ekron-native-test-save.txt";
    assert(world.save(path));
    sandbox::World loaded;
    assert(loaded.load(path));
    assert(loaded.objects.size()==1);
    assert(loaded.remove());
    assert(loaded.objects.empty());
    {std::ofstream bad(path);bad<<"EKRON_SANDBOX_1\n501\n";}
    assert(!loaded.load(path));
    {std::ofstream bad(path);bad<<"EKRON_SANDBOX_1\n1\n99 0 0 0\n";}
    assert(!loaded.load(path));
    assert(loaded.objects.empty());
    // Walk through the centre of an actual building entrance.
    loaded.x=-20;loaded.z=26;
    for(int i=0;i<120;i++)loaded.move(0,1,.016f);
    assert(loaded.z<22);
    // But a front wall blocks the same movement outside that opening.
    loaded.x=-24;loaded.z=26;
    for(int i=0;i<120;i++)loaded.move(0,1,.016f);
    assert(loaded.z>23);
    std::remove(path.c_str());
}
