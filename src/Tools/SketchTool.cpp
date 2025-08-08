#include "SketchTool.h"

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

void SketchTool::onMouseDown(int x,int y){ if(!geometry||!camera) return; Vector3 p; if(!screenToGround(camera,x,y,p)) return; drawing=true; pts.clear(); pts.emplace_back(p.x,0.0f,p.z); }
void SketchTool::onMouseMove(int x,int y){ if(!drawing||!geometry||!camera) return; Vector3 p; if(!screenToGround(camera,x,y,p)) return; Vector3 np(p.x,0.0f,p.z); if(pts.empty()||(np-pts.back()).length()>0.1f) pts.push_back(np); }
void SketchTool::onMouseUp(int,int){ if(!drawing||!geometry) return; drawing=false; if(pts.size()>2){ if((pts.back()-pts.front()).length()<0.5f) pts.back()=pts.front(); } if(pts.size()>=2) geometry->addCurve(pts); pts.clear(); }
