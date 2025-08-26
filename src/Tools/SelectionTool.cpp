#include "SelectionTool.h"
#include <limits>
#include <cmath>
#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/Solid.h"

static bool rayGround(CameraController* cam,int sx,int sy,Vector3& out){
    float cx,cy,cz; cam->getCameraPosition(cx,cy,cz);
    float yaw=cam->getYaw(), pitch=cam->getPitch();
    float ry=yaw*(float)M_PI/180.0f, rp=pitch*(float)M_PI/180.0f;
    Vector3 f(-sinf(ry)*cosf(rp), -sinf(rp), -cosf(ry)*cosf(rp)); f=f.normalized();
    Vector3 up(0,1,0); Vector3 r=f.cross(up).normalized(); up=r.cross(f).normalized();
    const float fov=60.0f; const int W=800,H=600; float aspect=(float)W/H;
    float nx=(2.0f*sx/W)-1.0f, ny=1.0f-(2.0f*sy/H); float th=tanf((fov*(float)M_PI/180.0f)/2.0f);
    Vector3 dir=(f + r*(nx*th*aspect) + up*(ny*th)).normalized(); Vector3 o(cx,cy,cz);
    if (fabs(dir.y) < 1e-6f) return false; float t=-o.y/dir.y; if (t<0) return false; out = o + dir*t; return true;
}

GeometryObject* SelectionTool::pickObjectAt(int sx,int sy){
    // For now pick nearest curve vertex or any solid bounding cylinder by distance
    Vector3 hit; if(!rayGround(camera,sx,sy,hit)) return nullptr;
    float bestD = std::numeric_limits<float>::max();
    GeometryObject* best=nullptr;
    for (const auto& uptr : geometry->getObjects()) {
        if (uptr->getType()==ObjectType::Curve) {
            const Curve* c = static_cast<const Curve*>(uptr.get());
            for (const auto& p : c->getPoints()) {
                float dx=p.x-hit.x, dz=p.z-hit.z;
                float d=dx*dx+dz*dz;
                if (d<bestD) { bestD=d; best=uptr.get(); }
            }
        } else {
            // prefer solids if close
            float dx=hit.x, dz=hit.z; float d=dx*dx+dz*dz;
            if (d<bestD) { bestD=d; best=uptr.get(); }
        }
    }
    return best;
}

void SelectionTool::onMouseDown(int x,int y){
    auto* obj = pickObjectAt(x,y);
    if (obj) {
        if (lastSelected) lastSelected->setSelected(false);
        obj->setSelected(true);
        lastSelected = obj;
    }
}

void SelectionTool::onMouseUp(int,int){}

void SelectionTool::onKeyPress(char key){
    if ((key==127 || key==8) && lastSelected) { // delete/backspace
        geometry->deleteObject(lastSelected);
        lastSelected = nullptr;
    }
}
