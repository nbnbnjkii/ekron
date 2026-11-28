#pragma once
#include "CollisionMesh.h"
#include <memory>

namespace physics {
class PlayerController {
    Vec3 feet_{};
    float velocity_=0,accumulator_=0,height_=1.8f,eye_=1.7f,camera_=1.7f;
    bool initialized_=false,grounded_=false,jump_=false;
    static constexpr float radius=.25f;
    float minX_=-1e6f,maxX_=1e6f,minZ_=-1e6f,maxZ_=1e6f;
    void confine(){feet_.x=std::clamp(feet_.x,minX_+radius,maxX_-radius);feet_.z=std::clamp(feet_.z,minZ_+radius,maxZ_-radius);}
    Bounds bodyBounds(Vec3 feet,float height)const{Bounds b;b.add(feet+Vec3{-radius,-.02f,-radius});b.add(feet+Vec3{radius,height+.02f,radius});return b;}
    bool contacts(const CollisionMesh&mesh,Vec3 feet,float height)const{
        bool hit=false;mesh.query(bodyBounds(feet,height),[&](const Triangle&t){Vec3 n;float d=capsuleTriangle(feet+Vec3{0,radius,0},feet+Vec3{0,height-radius,0},t,n);if(d<radius-.002f)hit=true;});return hit;
    }
    bool tryStep(const CollisionMesh&mesh,const CollisionMesh*extra,Vec3 from,Vec3 target){
        constexpr float maxStep=.75f;
        auto blocked=[&](Vec3 p){return contacts(mesh,p,height_)||(extra&&contacts(*extra,p,height_));};
        if(!blocked(target))return false;
        Vec3 direction=target-from;direction.y=0;float distance=length(direction);
        if(distance<1e-6f)return false;
        direction=direction*(1/distance);
        float support=-std::numeric_limits<float>::infinity();
        // Look under the leading edge of the capsule, not across a large gap or through a wall.
        for(float reach:{0.f,radius+.035f}){
            Vec3 probe=target+direction*reach;
            float h=mesh.groundHeight(probe.x,probe.z,from.y+maxStep,from.y+.025f);
            if(extra)h=std::max(h,extra->groundHeight(probe.x,probe.z,from.y+maxStep,from.y+.025f));
            support=std::max(support,h);
        }
        if(!std::isfinite(support))return false;
        float rise=support-from.y+.002f;
        if(rise<=.025f||rise>maxStep+.003f)return false;
        // Sweep upward first, then horizontally. A low lintel rejects the whole step.
        int slices=int(std::ceil(rise/.035f));
        for(int i=1;i<=slices;i++)if(blocked(from+Vec3{0,rise*i/slices,0}))return false;
        target.y=from.y+rise;
        if(blocked(target))return false;
        feet_=target;velocity_=0;grounded_=true;
        return true;
    }
    void solve(const CollisionMesh&mesh){
        for(int iteration=0;iteration<6;iteration++){
            bool hit=false;auto bounds=bodyBounds(feet_,height_);
            mesh.query(bounds,[&](const Triangle&t){
                Vec3 n;float d=capsuleTriangle(feet_+Vec3{0,radius,0},feet_+Vec3{0,height_-radius,0},t,n);
                if(d>=radius-.0001f)return;
                float depth=radius-d+.0002f;hit=true;
                bool walkableFace=std::abs(normalized(cross(t.b-t.a,t.c-t.a)).y)>.64f;
                if(walkableFace&&n.y>.64f&&feet_.y>=std::min({t.a.y,t.b.y,t.c.y})-.3f){
                    feet_.y+=depth/n.y;grounded_=true;if(velocity_<0)velocity_=0;
                }else if(walkableFace&&n.y<-.64f){feet_.y-=depth/-n.y;if(velocity_>0)velocity_=0;}
                else{float horizontal=std::hypot(n.x,n.z);if(horizontal>.001f){feet_.x+=n.x/horizontal*depth;feet_.z+=n.z/horizontal*depth;}}
            });
            if(!hit)break;
        }
    }
public:
    void setBoundary(float minX,float maxX,float minZ,float maxZ){minX_=minX;maxX_=maxX;minZ_=minZ;maxZ_=maxZ;}
    void reset(float x,float eyeY,float z){feet_={x,eyeY-eye_,z};velocity_=accumulator_=0;camera_=eyeY;grounded_=false;initialized_=true;jump_=false;}
    void jump(){jump_=true;}
    bool grounded()const{return grounded_;}
    Vec3 feet()const{return feet_;}
    float eyeHeight()const{return eye_;}
    bool overlaps(const CollisionMesh&mesh,float x,float eyeY,float z)const{return contacts(mesh,{x,eyeY-1.7f,z},1.8f);}
    void update(const CollisionMesh&mesh,float right,float forward,float yaw,float dt,bool crouched,float &x,float &y,float &z,const CollisionMesh *extra=nullptr,bool prone=false,const CollisionMesh *fenceBarriers=nullptr,bool sprint=false,bool jetFlight=false,float pitch=0){
        if(!initialized_)reset(x,y,z);
        const bool poweredFlight=jetFlight&&std::hypot(right,forward)>.1f;
        // Crouch keeps the feet still; standing up is refused under a concrete frame/ceiling.
        float desired=prone?.55f:crouched?1.f:1.8f;
        if(desired<=height_||(!contacts(mesh,feet_,desired)&&(!extra||!contacts(*extra,feet_,desired)))){
            height_=desired;eye_=prone?.4f:crouched?.9f:1.7f;
        }
        accumulator_+=std::clamp(dt,0.f,.05f);
        constexpr float step=1.f/120;
        while(accumulator_>=step){
            accumulator_-=step;bool wasGrounded=grounded_;
            if(jump_&&grounded_&&!prone&&!poweredFlight){velocity_=6.5f;grounded_=false;wasGrounded=false;}jump_=false;
            float lengthInput=std::max(1.f,std::hypot(right,forward)),speed=poweredFlight?(sprint?12.f:8.f):height_<.6f?1.f:height_<1.5f?2.2f:sprint?5.8f:2.7f;
            float flightPitch=std::isfinite(pitch)?std::clamp(pitch,-1.57079633f,1.57079633f):0.f;
            float horizontalForward=forward*(poweredFlight?std::cos(flightPitch):1.f);
            Vec3 before=feet_;
            feet_.x+=(std::cos(yaw)*right-std::sin(yaw)*horizontalForward)/lengthInput*speed*step;
            feet_.z+=(-std::sin(yaw)*right-std::cos(yaw)*horizontalForward)/lengthInput*speed*step;
            bool stepped=!poweredFlight&&wasGrounded&&velocity_<=0&&height_>.6f&&tryStep(mesh,extra,before,feet_);
            if(poweredFlight){
                // Fly along the camera's forward axis, but retain the capsule solver.
                // Releasing the stick stops lift and restores ordinary gravity.
                velocity_=0;feet_.y+=std::sin(flightPitch)*forward/lengthInput*speed*step;grounded_=false;
            }else if(!stepped){velocity_=std::max(-30.f,velocity_-18.f*step);feet_.y+=velocity_*step;grounded_=false;}
            confine();solve(mesh);if(extra)solve(*extra);if(fenceBarriers)solve(*fenceBarriers);confine();
            // Follow small downhill changes, but NEVER snap over an actual pit or from mid-air.
            if(!poweredFlight&&wasGrounded&&!grounded_&&velocity_<=0){
                float floor=mesh.groundHeight(feet_.x,feet_.z,feet_.y+.08f,feet_.y-.18f);
                if(std::isfinite(floor)&&feet_.y-floor<.18f){feet_.y=floor+.0005f;solve(mesh);grounded_=true;velocity_=0;}
            }
        }
        x=feet_.x;z=feet_.z;
        // Only camera Y is damped. Physics is fixed-step and never depends on camera smoothing.
        float target=feet_.y+eye_;
        camera_+=(target-camera_)*(1-std::exp(-22.f*std::clamp(dt,0.f,.05f)));
        if(std::abs(camera_-target)<.0001f)camera_=target;
        y=camera_;
    }
};
}
