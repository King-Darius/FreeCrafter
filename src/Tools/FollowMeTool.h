#pragma once

#include "Tool.h"

#include <vector>

class FollowMeTool : public Tool {
public:
    FollowMeTool(GeometryKernel* geometry, CameraController* camera);

    const char* getName() const override { return "FollowMeTool"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    PreviewState buildPreview() const override;

private:
    std::vector<Curve*> gatherSelectedCurves() const;
    void applySweep(Curve* profile, Curve* path);
    void reset();

    std::vector<Curve*> lastSelection;
};
