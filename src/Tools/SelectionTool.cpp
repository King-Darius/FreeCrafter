#include "SelectionTool.h"
#include <cmath>

GeometryObject* SelectionTool::pickObjectAt(int screenX, int screenY) {
    if (!geometry || !camera) return nullptr;
    float camX, camY, camZ; camera->getCameraPosition(camX,camY,camZ);
    float yaw=camera->getYaw(), pitch=camera->getPitch();
    float ry = yaw * (float)M_PI/180.0f, rp = pitch*(float)M_PI/180.0f;
    Vector3 f(-sinf(ry)*cosf(rp), -sinf(rp), -cosf(ry)*cosf(rp)); f=f.normalized();
    Vector3 up(0,1,0); Vector3 r = f.cross(up).normalized(); up = r.cross(f).normalized();
    const float fov=60.0f; const int W=800,H=600; float aspect=(float)W/H;
    float nx= (2.0f*screenX/W)-1.0f; float ny= 1.0f-(2.0f*screenY/H);
    float th = tanf((fov*(float)M_PI/180.0f)/2.0f);
    Vector3 dir = (f + r*(nx*th*aspect) + up*(ny*th)).normalized();
    Vector3 orig(camX,camY,camZ);
    if (fabs(dir.y) < 1e-6f) return nullptr; float t = -orig.y/dir.y; if (t < 0) return nullptr;
    Vector3 hit = orig + dir*t; GeometryObject* best=nullptr; float bestDist=1e9f;
    for (auto& o : geometry->getObjects()) {
        if (o->getType()==ObjectType::Curve){
            const auto& pts = static_cast<Curve*>(o.get())->getPoints();
            for(size_t i=0;i+1<pts.size();++i){
                Vector3 A=pts[i], B=pts[i+1], AB=B-A, AP=hit-A; float d=AB.dot(AB);
                float u = d>1e-6f ? AP.dot(AB)/d : 0; if(u<0)u=0; if(u>1)u=1;
                Vector3 C = A + AB*u; float dist = (hit-C).length();
                if (dist < 0.5f) { float hd=(orig-hit).length(); if(hd<bestDist){bestDist=hd; best=o.get();} break; }
            }
        } else if (o->getType()==ObjectType::Solid){
            const auto& V = static_cast<Solid*>(o.get())->getVertices();
            std::vector<Vector3> base; for (auto& v:V) if (fabs(v.y)<1e-6f) base.push_back(v);
            bool inside=false; for(size_t i=0,j=base.size()-1;i<base.size();j=i++){
                float xi=base[i].x, zi=base[i].z, xj=base[j].x, zj=base[j].z;
                bool inter=((zi>hit.z)!=(zj>hit.z)) && (hit.x < (xj-xi)*(hit.z-zi)/(zj-zi)+xi);
                if (inter) inside=!inside;
            }
            if (inside){ float hd=(orig-hit).length(); if(hd<bestDist){bestDist=hd; best=o.get();} }
        }
    }
    return best;
}

void SelectionTool::onMouseDown(int x, int y) {
    GeometryObject* obj = pickObjectAt(x,y);
    for (auto& o : geometry->getObjects()) o->setSelected(false);
    lastSelected = nullptr; if (obj){ obj->setSelected(true); lastSelected=obj; }
}

void SelectionTool::onMouseUp(int, int) {}

void SelectionTool::onKeyPress(char key) {
    if (key == 127 || key == '\b' || key == 0x7F) {
        if (lastSelected) { geometry->deleteObject(lastSelected); lastSelected=nullptr; }
    }
}
