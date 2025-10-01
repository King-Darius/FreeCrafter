#pragma once

#include "../GeometryKernel/GeometryObject.h"
#include "../GeometryKernel/Vector3.h"
#include "../CameraController.h"
#include "../Interaction/InferenceEngine.h"

#include <vector>

struct BoundingBox {
    Vector3 min;
    Vector3 max;
    bool valid = false;
};

BoundingBox computeBoundingBox(const GeometryObject& object);
Vector3 computeCentroid(const GeometryObject& object);
void translateObject(GeometryObject& object, const Vector3& delta);
void rotateObject(GeometryObject& object, const Vector3& pivot, const Vector3& axis, float angleRadians);
void scaleObject(GeometryObject& object, const Vector3& pivot, const Vector3& factors);
bool projectScreenToGround(CameraController* camera, int x, int y, int viewportWidth, int viewportHeight, Vector3& out);
Vector3 snapPlanarToGrid(const Vector3& point, float grid = 0.25f, float epsilon = 0.08f);
bool resolvePlanarPoint(const Interaction::InferenceResult& snap,
    CameraController* camera,
    int x,
    int y,
    int viewportWidth,
    int viewportHeight,
    Vector3& out);
bool pointInPolygonXZ(const std::vector<Vector3>& polygon, const Vector3& point, float tolerance = 1e-4f);
bool offsetPolygon(const std::vector<Vector3>& polygon, float distance, std::vector<Vector3>& out);

