#pragma once
#include "SandboxWorld.h"
#include "CollisionMesh.h"
#include "ObjectCcd.h"
#include "ObjectOverlap.h"
#include "BodyContacts.h"
#include "CarSuspension.h"
#include "PairSolver.h"
#include "SupportGeometry.h"
#include "CityDepenetration.h"
namespace sandbox {
// Lightweight box dynamics, deliberately separate from the immutable city triangle BVH.
class ObjectDynamics {
 float accumulator_=0;
 BodyBroadphase broadphase_;
 BodyBroadphase pairBroadphase_;
public:
 std::vector<physics::Vec3> barrelImpacts;
 PairSolverStats pairStats{};
 size_t neighbourMeshBuilds=0;
 bool update(World&w,const physics::CollisionMesh&mesh,float dt,int held,const std::array<float,3>&lo,const std::array<float,3>&hi,int*heldIndex=nullptr,int*seatIndex=nullptr){
  barrelImpacts.clear();
  accumulator_+=std::clamp(dt,0.f,.05f);bool changed=false;constexpr float step=1.f/120;
  while(accumulator_>=step){accumulator_-=step;
   // Validate all sort keys and geometry inputs BEFORE physicalOrder/BVH.
   // NaN angles break strict ordering; non-finite velocities poison sweeps.
   for(auto&o:w.objects)sanitizeBodyState(o);
   const auto beforeBodies=w.objects;
   const auto order=physicalOrder(w);
   broadphase_.build(w,held,step);
   for(size_t index:order){
    auto&o=w.objects[index];
    if(!std::isfinite(o.x)||!std::isfinite(o.y)||!std::isfinite(o.z)||o.x<lo[0]||o.x>hi[0]||o.z<lo[2]||o.z>hi[2]||o.y<std::max(-50.f,lo[1]-30)){
     if(int(index)!=held)o.destroyed=true;
    }
    if(o.destroyed)continue;
    if(int(index)==held||o.anchored||o.frozen)continue;
    if(!o.awake){o.supportCheckTime+=step;if(o.supportCheckTime<.2f)continue;o.supportCheckTime=0;}

    auto ground=[&](float x,float z,float top,float bottom){
     float height=mesh.groundHeight(x,z,top,bottom);
     for(size_t j:broadphase_.neighbours[index])if(!w.objects[j].destroyed){physics::CollisionMesh::RayHit hit;
      if(broadphase_.solid(w,j).groundContact(x,z,top,bottom,hit,true))height=std::max(height,hit.position.y);}
     return height;
    };
    // Sleeping support validation is read-only. Previously every validation
    // also ran depenetration/impulses, moving an otherwise resting body upward.
    if(o.awake)recoverCityContact(o,mesh);
    std::vector<physics::Vec3>citySupport;
    bool cityWall=o.awake?solveCityContactVelocities(o,mesh,&citySupport):false;
    Object beforeStep=o;
    auto b=World::bounds(o);physics::Vec3 half{b.w/2,b.h/2,b.d/2};
    auto radius=[&](){return std::abs(std::sin(o.tiltZ))*half.x+std::abs(std::cos(o.tiltZ)*std::cos(o.tiltX))*half.y+std::abs(std::cos(o.tiltZ)*std::sin(o.tiltX))*half.z;};
    if(!o.awake){
     bool supported=false;
     for(float cx:{-1.f,0.f,1.f})for(float cy:{-1.f,0.f,1.f})for(float cz:{-1.f,0.f,1.f}){
      if(!lowerBodySample(o,cx,cy,cz))continue;
      auto p=bodyContactPoint(o,cx,cy,cz);
      float h=ground(p.x,p.z,p.y+.025f,p.y-.035f);
      if(std::isfinite(h)&&std::abs(p.y-h)<.035f)supported=true;
     }
     if(o.type==8&&stepSuspension(o,0,ground,false).contacts>=2)supported=true;
     if(!supported)for(size_t j:broadphase_.neighbours[index])if(!w.objects[j].destroyed){const auto&other=w.objects[j];
      for(float x:{-1.f,0.f,1.f})for(float yy:{-1.f,0.f,1.f})for(float z:{-1.f,0.f,1.f})if(lowerBodySample(other,-x,-yy,-z)){
       auto p=bodyContactPoint(other,x,yy,z);float bottom=b.y-radius()-.1f,limit=2*radius()+.2f;
       float distance=World::rayDistance(o,p.x,bottom,p.z,0,1,0,limit);if(distance>=limit)continue;
       float bodyY=bottom+distance,h=ground(p.x,p.z,bodyY+.025f,bodyY-.035f);if(std::isfinite(h)&&std::abs(bodyY-h)<.035f)supported=true;
      }
     }
     if(supported){o.vx=o.vy=o.vz=o.spinX=o.spinZ=0;continue;}
     o.awake=true;o.restTime=0;
    }
    auto collides=[&](float x,float y,float z){
     physics::Vec3 center{x,y+half.y,z};
     Object candidate=o;candidate.x=x;candidate.y=y;candidate.z=z;
     if(objectOverlap(mesh,candidate,center))return true;
     return broadphase_.overlaps(w,index,candidate,center);
    };
    auto impact=[&](bool axisX,float x,float z){
     float &velocity=axisX?o.vx:o.vz;
     Object candidate=o;candidate.x=x;candidate.z=z;
     for(size_t j:broadphase_.neighbours[index])if(!w.objects[j].destroyed){
      auto &other=w.objects[j];
      if(!bodiesOverlap(candidate,other))continue;
      float &otherVelocity=axisX?other.vx:other.vz;
      float direction=(axisX?other.x-o.x:other.z-o.z)>=0?1.f:-1.f;
      float closing=(velocity-otherVelocity)*direction;
      if(closing<=0)continue;
      float inverse=1/World::mass(o),inverseOther=other.frozen||other.anchored?0:1/World::mass(other);
      float impulse=(1+contactMaterial(o).restitution)*closing/(inverse+inverseOther);
      velocity-=direction*impulse*inverse;
      if(inverseOther>0){otherVelocity+=direction*impulse*inverseOther;other.awake=true;other.restTime=0;}
      return;
     }
     velocity=std::abs(velocity)<.5f?0:-velocity*.35f;
    };
    float nx=o.x+o.vx*step,nz=o.z+o.vz*step;
    if(collides(nx,o.y,o.z))impact(true,nx,o.z);else o.x=nx;
    if(collides(o.x,o.y,nz))impact(false,o.x,nz);else o.z=nz;
    const float airDrag=std::exp(-.015f*step),angularDrag=std::exp(-.1f*step);
    o.vx*=airDrag;o.vz*=airDrag;o.spinX*=angularDrag;o.spinZ*=angularDrag;
    auto suspension=stepSuspension(o,step,ground);
    o.vy=std::max(-25.f,o.vy-18.f*step);float nextY=o.y+o.vy*step;
    o.tiltX+=o.spinX*step;o.tiltZ+=o.spinZ*step;
    float verticalRadius=radius();
    // A surface below the upright footprint is NOT necessarily below the
    // tilted body. Compute the permitted centre height from actual corners.
    float actualSupport=-1e6f;physics::Vec3 contactPoint{};int contactCount=0;std::vector<physics::Vec3> supportPoints;std::vector<std::pair<float,physics::Vec3>> supportCandidates;
    for(float cx:{-1.f,0.f,1.f})for(float cy:{-1.f,0.f,1.f})for(float cz:{-1.f,0.f,1.f}){
     if(!lowerBodySample(o,cx,cy,cz))continue;
     auto p=bodyContactPoint(o,cx,cy,cz);
     float h=ground(p.x,p.z,p.y+.04f,lo[1]-20);
     if(std::isfinite(h)){float candidate=h-(p.y-(o.y+half.y-verticalRadius));actualSupport=std::max(actualSupport,candidate);supportCandidates.push_back({candidate,p});}
    }
    // Contact points from the support as well as the supported body. A wide
    // panel can bridge two narrow columns even when its own sample grid misses
    // both columns; SAT already detects the contact, so support must agree.
    auto sampleSupport=[&](float px,float pz){
     float bottom=o.y+half.y-verticalRadius-.1f,limit=2*verticalRadius+.2f;
     float distance=World::rayDistance(o,px,bottom,pz,0,1,0,limit);
     if(distance>=limit)return;
     physics::Vec3 point{px,bottom+distance,pz};float h=ground(px,pz,point.y+.04f,lo[1]-20);
     if(std::isfinite(h)){float candidate=h-(point.y-(o.y+half.y-verticalRadius));actualSupport=std::max(actualSupport,candidate);supportCandidates.push_back({candidate,point});}
    };
    for(size_t j:broadphase_.neighbours[index])if(!w.objects[j].destroyed){const auto&other=w.objects[j];
     for(float x:{-1.f,0.f,1.f})for(float y:{-1.f,0.f,1.f})for(float z:{-1.f,0.f,1.f})if(lowerBodySample(other,-x,-y,-z)){
      auto point=bodyContactPoint(other,x,y,z);sampleSupport(point.x,point.z);
     }
    }
    physics::Bounds supportArea;
    for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})supportArea.add(bodyContactPoint(o,x,y,z));
    supportArea.lo=supportArea.lo-physics::Vec3{.02f,.08f,.02f};
    mesh.query(supportArea,[&](const physics::Triangle&t){auto n=physics::normalized(physics::cross(t.b-t.a,t.c-t.a));if(std::abs(n.y)<.64f)return;
     for(auto point:{t.a,t.b,t.c,physics::closestTriangle({o.x,b.y-verticalRadius,o.z},t)})sampleSupport(point.x,point.z);
    });
    for(const auto&candidate:supportCandidates)if(candidate.first>=actualSupport-.02f){contactPoint=contactPoint+candidate.second;contactCount++;supportPoints.push_back(candidate.second);}
    if(contactCount>0)contactPoint=contactPoint*(1.f/contactCount);
    float support=actualSupport;
    supportPoints.insert(supportPoints.end(),citySupport.begin(),citySupport.end());
    if(!supportPoints.empty()){contactPoint={};for(auto p:supportPoints)contactPoint=contactPoint+p;contactPoint=contactPoint*(1.f/supportPoints.size());}
    bool balanced=comInSupport(supportPoints,{o.x,b.y,o.z});
    if(!balanced&&o.vy<=0&&nextY+half.y-verticalRadius<=actualSupport+.02f){
     // Gravity about the real support, with the parallel-axis inertia.
     float rx=contactPoint.x-o.x,ry=contactPoint.y-b.y,rz=contactPoint.z-o.z;
     ContactInertia inertia(o);
     o.spinX-=18.f*rz/(inertia.ix+ry*ry+rz*rz)*step;
     o.spinZ+=18.f*rx/(inertia.iz+ry*ry+rx*rx)*step;
    }

    bool restingContact=false;
    if(o.vy<=0&&support>-1e5f&&nextY+half.y-verticalRadius<=support+.012f){
     restingContact=true;
     // No global angular damping at impact. Static contact can hold an
     // already balanced patch, but cannot slow an unbalanced fall.
     if(balanced&&std::hypot(o.spinX,o.spinZ)<.03f)o.spinX=o.spinZ=0;
     // The chassis/box contact also constrains angular motion. Otherwise a
     // spring can accumulate roll into the same hard stop on every step.
     float tx=-(contactPoint.z-o.z),tz=contactPoint.x-o.x;
     float ix=(b.h*b.h+b.d*b.d)/12,iz=(b.w*b.w+b.h*b.h)/12;
     float incoming=tx*o.spinX+tz*o.spinZ,denom=tx*tx/ix+tz*tz/iz;
     if(incoming<0&&denom>1e-7f){float impulse=-incoming/denom;o.spinX+=tx*impulse/ix;o.spinZ+=tz*impulse/iz;}
     o.vy=std::abs(o.vy)>2?-o.vy*contactMaterial(o).restitution:0;
     // Rest on the current rotated body, not on an upright replacement.
     o.y=support+verticalRadius-half.y+.005f;
     // Contact is dissipative. Terrain gradients must not continuously inject
     // angular energy into a stationary car (the former levitation feedback).
     applyPatchFriction(o,supportPoints,step);

    }else if(o.vy>0&&collides(o.x,nextY,o.z))o.vy*=-.35f;
    else o.y=nextY;
    // A validated slow resting correction already resolves support. Sweeping
    // its angular chord again could rewind it to t=0 on the same floor forever.
    bool resolvedRest=restingContact&&std::abs(beforeStep.vy)<2&&std::hypot(o.vx,o.vz)<.5f
      &&!objectOverlap(mesh,o,{o.x,o.y+half.y,o.z})&&!broadphase_.overlaps(w,index,o,{o.x,o.y+half.y,o.z});
    if(!resolvedRest){
     sweepObjectFloor(beforeStep,o,mesh,true);
     for(size_t j:broadphase_.neighbours[index])if(!w.objects[j].destroyed)sweepObjectFloor(beforeStep,o,broadphase_.solid(w,j),true);
    }
    if(o.type==8&&suspension.contacts>0){
     applyContactFriction(o,step*std::clamp(suspension.force/(World::mass(o)*18),0.f,1.f));
     if(std::abs(o.vy)<.025f&&std::hypot(o.vx,o.vz)<.025f&&std::hypot(o.spinX,o.spinZ)<.1f&&std::abs(o.tiltX-beforeStep.tiltX)+std::abs(o.tiltZ-beforeStep.tiltZ)<.001f&&suspension.contacts>=2&&suspension.force>0){
      o.restTime+=step;if(o.restTime>.5f){o.vx=o.vy=o.vz=o.spinX=o.spinZ=0;o.awake=false;}
     }else o.restTime=0;
    }
    // One bounded rescue only, onto a real nearby surface. Do not turn genuine
    // pits into a global invisible floor or reintroduce an endless respawn loop.
    if(o.y< -5&&!o.floorRecoveryUsed){
     float bottom=o.y+half.y-radius();
     float below=ground(o.x,o.z,bottom+.08f,lo[1]-20);
     float surface=ground(o.x,o.z,std::min(3.f,bottom+12),bottom+.1f);
     if(!std::isfinite(below)&&std::isfinite(surface)){
      o.y=surface+radius()-half.y+.02f;o.vx=o.vy=o.vz=o.spinX=o.spinZ=0;o.floorRecoveryUsed=true;o.restTime=0;
     }
    }
    // Only a real deep embedding re-wakes a settling body. Sub-centimetre seam
    // corrections may reposition the frame but must not keep resetting rest,
    // which is what made thrown props hover/bounce instead of lying flat.
    if(recoverCityContact(o,mesh)>.01f){o.awake=true;o.restTime=0;}
    solveCityContactVelocities(o,mesh);
    // Judge rest only after every normal/friction impulse has been solved.
    // Intermediate velocities before the manifold solve are not physical rest
    // velocities (in particular at a sloped triangle seam). A body balancing on
    // a corner/edge has balanced=false but is still genuinely at rest once its
    // motion is frozen; otherwise it vibrates forever instead of sleeping.
    if(o.type!=8||suspension.contacts==0){
     float linearSpeed=std::hypot(o.vx,o.vz),angularSpeed=std::hypot(o.spinX,o.spinZ);
     bool frozenInPlace=linearSpeed<.015f&&std::abs(o.vy)<.015f&&angularSpeed<.008f;
     if(restingContact&&(balanced||cityWall||frozenInPlace)&&linearSpeed<.025f&&std::abs(o.vy)<.025f&&angularSpeed<.03f){
      o.restTime+=step;if(o.restTime>.35f){o.vx=o.vy=o.vz=o.spinX=o.spinZ=0;o.awake=false;}
     }else o.restTime=0;
    }
    if(o.y<std::max(-50.f,lo[1]-30)||o.x<lo[0]||o.x>hi[0]||o.z<lo[2]||o.z>hi[2])o.destroyed=true;
    changed=true;
   }
   neighbourMeshBuilds=broadphase_.meshBuilds;
   pairStats=solveBodyPairs(w,mesh,pairBroadphase_,beforeBodies,held,step);
   if(pairStats.contacts||pairStats.sweeps)changed=true;
   for(auto&o:w.objects)sanitizeBodyState(o);
   for(size_t i=0;i<w.objects.size();i++){auto&o=w.objects[i];if(o.type!=29||o.destroyed||int(i)==held)continue;
    const auto&old=beforeBodies[i];float impact=physics::length({o.vx-old.vx,o.vy-old.vy,o.vz-old.vz});
    if(impact>=5.5f){auto b=World::bounds(o);barrelImpacts.push_back({b.x,b.y,b.z});o.destroyed=true;changed=true;}
   }
  }
  // Reverse erasure preserves surviving state and repairs live grab/seat indices.
  for(size_t i=w.objects.size();i-->0;)if(w.objects[i].destroyed){
   auto repair=[&](int*p){if(p){if(*p==int(i))*p=-1;else if(*p>int(i))--*p;}};
   repair(heldIndex);repair(seatIndex);w.objects.erase(w.objects.begin()+i);changed=true;
  }
  return changed;
 }
};
}
