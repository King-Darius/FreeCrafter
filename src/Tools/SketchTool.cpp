#include "SketchTool.h"
#include <cmath>

static bool screenToGround(CameraController* cam,int x,int y,Vector3& out){
    float cx,cy,cz; cam->getCameraPosition(cx,cy,cz);
    float yaw=cam->getYaw(), pitch=cam->getPitch();
    float ry=yaw*(float)M_PI/180.0f, rp=pitch*(float)M_PI/180.0f;
    Vector3 f(-sinf(ry)*cosf(rp), -sinf(rp), -cosf(ry)*cosf(rp)); f=f.normalized();
    Vector3 up(0,1,0); Vector3 r=f.cross(up).normalized(); up=r.cross(f).normalized();
    const float fov=60.0f; const int W=800,H=600; float aspect=(float)W/H;
    float nx=(2.0f*x/W)-1.0f, ny=1.0f-(2.0f*y/H); float th=tanf((fov*(float)M_PI/180.0f)/2.0f);
    Vector3 dir=(f + r*(nx*th*aspect) + up*(ny*th)).normalized(); Vector3 o(cx,cy,cz);
    if (fabs(dir.y) < 1e-6f) return false; float t=-o.y/dir.y; if (t<0) return false; out = o + dir*t; return true;
}

static void axisSnap(Vector3& p){
    // snap near integer grid and axis align (very lightweight)
    const float grid=0.25f, eps=0.08f;
    float gx = std::round(p.x/grid)*grid;
    float gz = std::round(p.z/grid)*grid;
    if (std::fabs(p.x-gx) < eps) p.x = gx;
    if (std::fabs(p.z-gz) < eps) p.z = gz;
}

void SketchTool::onMouseDown(int x,int y){
    Vector3 p; if(!screenToGround(camera,x,y,p)) return;
    axisSnap(p);
    if (!drawing) {
        drawing = true; pts.clear(); pts.push_back(Vector3(p.x,0,p.z));
    } else {
        pts.push_back(Vector3(p.x,0,p.z));
    }
}

void SketchTool::onMouseMove(int x,int y){
    if (!drawing) return;
    Vector3 p; if(!screenToGround(camera,x,y,p)) return;
    axisSnap(p);
    if (pts.empty()) pts.push_back(Vector3(p.x,0,p.z));
    if (pts.size()==1) {
        // preview second point
    } else {
        // live update last preview point
        if (pts.size()>1) pts.back() = Vector3(p.x,0,p.z);
    }
}

void SketchTool::onMouseUp(int,int){
    // if double-click–ish length >=2 finalize
    if (drawing && pts.size()>=2) {
        geometry->addCurve(pts);
        drawing=false; pts.clear();
    }
}
