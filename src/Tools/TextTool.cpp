#include "TextTool.h"

#include "ToolGeometryUtils.h"

#include "../Scene/Document.h"

TextTool::TextTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* doc)
    : Tool(geometry, camera)
    , document(doc)
{
}

void TextTool::onPointerDown(const PointerInput& input)
{
    if (!document)
        return;

    Vector3 point;
    if (!resolvePoint(input, point))
        return;

    Scene::TextAnnotation annotation;
    annotation.position = point;
    annotation.text = content;
    document->addTextAnnotation(annotation);
}

void TextTool::onPointerHover(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        hoverValid = false;
        return;
    }
    hoverPoint = point;
    hoverValid = true;
}

void TextTool::onCancel()
{
    hoverValid = false;
    setState(State::Idle);
}

Tool::PreviewState TextTool::buildPreview() const
{
    PreviewState state;
    if (!hoverValid)
        return state;

    PreviewPolyline poly;
    poly.points.push_back(hoverPoint);
    state.polylines.push_back(poly);
    return state;
}

bool TextTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}
