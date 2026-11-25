#pragma once
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <chrono>
#include "SandboxWorld.h"
#include "ToolInteraction.h"
#include "FpvDrone.h"
#include "ObjectDynamics.h"
#include "WorldRecovery.h"
#include "Interaction.h"
#include "RaycastPlacement.h"
#include "PlayerPhysics.h"
#include <future>
#include <optional>
#include "TextureLoader.h"
#include "GltfRenderer.h"
#include "MilitaryEffects.h"
#include "PlayerView.h"
#include "BlastEffects.h"
struct android_app;
class Renderer {
public:
    explicit Renderer(android_app *app);
    ~Renderer();
    void handleInput();
    void render();
    void resetInput();
private:
    android_app *app_;
    EGLDisplay display_=EGL_NO_DISPLAY;
    EGLSurface surface_=EGL_NO_SURFACE;
    EGLContext context_=EGL_NO_CONTEXT;
    GLuint program_=0,skyProgram_=0,buffer_=0,vao_=0;
    size_t dynamicCapacity_=0;
    void uploadDynamic(const std::vector<sandbox::MeshVertex>&vertices);
    TextureLoader textures_;
    military::Effects militaryEffects_;
    sandbox::PlayerView playerView_;
    sandbox::BlastEffects blasts_;
    bool crashSpectating_=false;
    float crashSpectateTime_=0;
    physics::Vec3 crashSpectateFrom_{},crashSpectateTarget_{};
    bool carChase_=false;
    float rotorPhase_=0;
    city::GltfRenderer city_;
    city::GltfRenderer car_;
    city::GltfRenderer remote_;
    city::GltfRenderer droneModel_;
    sandbox::FpvDrone drone_;
    bool texturesReady_=false;
    int width_=1,height_=1,leftId_=-1,rightId_=-1;
    float originX_=0,originY_=0,rightX_=0,rightY_=0,moveX_=0,moveY_=0;
    int seatedCar_=-1;
    float seatTime_=0;
    physics::Vec3 seatFrom_{};
    bool carWasAnchored_=false;
    std::optional<sandbox::Object> placementPreview_;
    bool building_=false;
    sandbox::HandTool spawnReturnTool_=sandbox::HandTool::HAND_EMPTY;
    bool flying_=false;
    sandbox::World world_;
    sandbox::ToolInteraction tools_;
    sandbox::ObjectDynamics dynamics_;
    float motionSaveTime_=0;
    sandbox::Object carParking_{8,0,0,0};
    bool carParkingValid_=false;
    void respawnPlayer();
    physics::PlayerController player_;
    bool crouched_=false,prone_=false;
    std::future<bool> saveJob_;
    bool saveDirty_=false;
    std::future<std::optional<std::array<float,2>>> roadJob_;
    physics::CollisionMesh builtCollision_;
    void rebuildBuiltCollision();
    struct PreparedAssets {city::Scene city,car,remote,drone;TextureLoader::Prepared textures;std::string log;float parkingX=0,parkingZ=0,parkingY=0;bool valid=false,carValid=false,remoteValid=false,droneValid=false;};
    std::future<PreparedAssets> assetsJob_;
    void advanceLoading();
    bool assetsInstalled_=false;
    std::string savePath_;
    std::chrono::steady_clock::time_point last_;
};
