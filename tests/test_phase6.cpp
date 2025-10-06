#include <cassert>
#include <vector>

#include "Scene/Document.h"
#include "GeometryKernel/Vector3.h"
#include "GeometryKernel/GeometryObject.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Solid.h"
#include "Phase6/AdvancedModeling.h"

using Scene::Document;
using Phase6::RoundCorner;
using Phase6::RoundCornerOptions;
using Phase6::CornerStyle;
using Phase6::CurveIt;
using Phase6::LoftOptions;
using Phase6::PushAndPull;
using Phase6::PushPullOptions;
using Phase6::Surface;
using Phase6::SurfaceDrawOptions;
using Phase6::BezierKnife;
using Phase6::KnifeOptions;
using Phase6::QuadTools;
using Phase6::QuadConversionOptions;
using Phase6::SubD;
using Phase6::SubdivisionOptions;
using Phase6::Weld;
using Phase6::WeldOptions;
using Phase6::VertexTools;
using Phase6::SoftSelectionOptions;
using Phase6::Clean;
using Phase6::CleanOptions;
using Phase6::ClothEngine;
using Phase6::ClothOptions;
using Phase6::CADDesigner;
using Phase6::RevolveOptions;
using Phase6::SweepOptions;
using Phase6::ShellOptions;
using Phase6::PatternOptions;
using Phase6::SplitOptions;

std::vector<Vector3> makeRectangle(float width, float depth)
{
    float hw = width * 0.5f;
    float hd = depth * 0.5f;
    return {
        { -hw, 0.0f, -hd },
        { hw, 0.0f, -hd },
        { hw, 0.0f, hd },
        { -hw, 0.0f, hd }
    };
}

void testRoundCornerAndWeld()
{
    Document doc;
    auto* obj = doc.geometry().addCurve(makeRectangle(2.0f, 2.0f));
    assert(obj);
    auto* curve = static_cast<Curve*>(obj);
    RoundCorner round(doc.geometry());
    RoundCornerOptions opts;
    opts.radius = 0.3f;
    opts.segments = 3;
    opts.style = CornerStyle::Chamfer;
    opts.tagHardEdges = true;
    opts.overrides.push_back({ 1, 0.15f, 2, CornerStyle::Fillet, false });
    bool ok = round.filletCurve(*curve, opts);
    assert(ok);
    assert(curve->getBoundaryLoop().size() > 4);
    const auto& hardness = curve->getEdgeHardness();
    assert(!hardness.empty());
    assert(hardness[0]);

    std::vector<Vector3> noisy = curve->getBoundaryLoop();
    noisy.push_back(noisy.front());
    assert(curve->rebuildFromPoints(noisy));
    Weld weld;
    WeldOptions weldOpts;
    weldOpts.tolerance = 0.05f;
    weldOpts.direction = Vector3(0.0f, 1.0f, 0.0f);
    weldOpts.directionalWeight = 0.5f;
    bool welded = weld.apply(*curve, weldOpts);
    assert(welded);
    assert(curve->getBoundaryLoop().size() < noisy.size());
}

void testCurveItAndPushPull()
{
    Document doc;
    Curve* base = static_cast<Curve*>(doc.geometry().addCurve(makeRectangle(2.0f, 1.5f)));
    assert(base);
    std::vector<Vector3> offset = makeRectangle(1.0f, 1.0f);
    for (auto& p : offset) {
        p.y = 1.5f;
        p.x *= 0.6f;
        p.z *= 0.6f;
    }
    Curve* top = static_cast<Curve*>(doc.geometry().addCurve(offset));
    assert(top);
    CurveIt loft(doc.geometry());
    LoftOptions loftOpts;
    loftOpts.sections = 4;
    loftOpts.twistDegrees = 15.0f;
    Solid* shell = loft.loft(*base, *top, loftOpts);
    assert(shell);
    std::size_t originalVerts = shell->getMesh().getVertices().size();
    assert(originalVerts > 0);

    PushAndPull thickener;
    PushPullOptions pushOpts;
    pushOpts.distance = 0.25f;
    pushOpts.createCaps = true;
    bool thickened = thickener.thicken(*shell, pushOpts);
    assert(thickened);
    assert(shell->getMesh().getVertices().size() >= originalVerts * 2);
}

void testSurfaceAndKnife()
{
    Document doc;
    Curve* base = static_cast<Curve*>(doc.geometry().addCurve(makeRectangle(1.0f, 1.0f)));
    assert(base);
    Solid* solid = static_cast<Solid*>(doc.geometry().extrudeCurve(base, 1.0f));
    assert(solid);

    Surface surface(doc.geometry());
    SurfaceDrawOptions drawOpts;
    drawOpts.projectionOffset = 0.01f;
    std::vector<Vector3> path = {
        { -0.4f, 0.5f, -0.4f },
        { 0.0f, 0.6f, 0.0f },
        { 0.4f, 0.5f, 0.4f }
    };
    Curve* scribble = surface.drawPolylineOnSolid(*solid, path, drawOpts);
    assert(scribble);
    assert(scribble->getBoundaryLoop().size() >= path.size());
    float minY = solid->getMesh().getVertices().front().position.y;
    float maxY = minY;
    for (const auto& v : solid->getMesh().getVertices()) {
        minY = std::min(minY, v.position.y);
        maxY = std::max(maxY, v.position.y);
    }
    for (const auto& p : scribble->getBoundaryLoop()) {
        assert(p.y >= minY - 1e-3f && p.y <= maxY + 1e-3f);
    }

    BezierKnife knife(doc.geometry());
    KnifeOptions knifeOpts;
    knifeOpts.samplesPerSegment = 8;
    knifeOpts.removeInterior = true;
    knifeOpts.extrusionHeight = 0.05f;
    knifeOpts.cutWidth = 0.2f;
    std::size_t beforeFaces = solid->getMesh().getFaces().size();
    std::vector<float> beforeHeights;
    beforeHeights.reserve(solid->getMesh().getVertices().size());
    for (const auto& v : solid->getMesh().getVertices())
        beforeHeights.push_back(v.position.y);
    Curve* cut = knife.cut(*solid, path, knifeOpts);
    assert(cut);
    assert(cut->getBoundaryLoop().size() > path.size());
    assert(solid->getMesh().getFaces().size() <= beforeFaces);
    bool lowered = false;
    const auto& afterVerts = solid->getMesh().getVertices();
    for (std::size_t i = 0; i < afterVerts.size() && i < beforeHeights.size(); ++i) {
        if (afterVerts[i].position.y < beforeHeights[i] - 1e-4f) {
            lowered = true;
            break;
        }
    }
    assert(lowered);
}

void testQuadSubDAndClean()
{
    Document doc;
    Curve* base = static_cast<Curve*>(doc.geometry().addCurve(makeRectangle(1.5f, 1.0f)));
    Solid* solid = static_cast<Solid*>(doc.geometry().extrudeCurve(base, 1.0f));
    assert(solid);

    QuadTools quads;
    QuadConversionOptions qopts;
    bool retopo = quads.retopologizeToQuads(*solid, qopts);
    assert(retopo);
    const auto& meshAfter = solid->getMesh();
    const auto& faces = meshAfter.getFaces();
    const auto& halfEdges = meshAfter.getHalfEdges();
    for (const auto& face : faces) {
        if (face.halfEdge < 0)
            continue;
        int count = 0;
        int start = face.halfEdge;
        int current = start;
        do {
            const auto& he = halfEdges[current];
            current = he.next;
            ++count;
        } while (current != start && current >= 0);
        assert(count == 4);
    }

    SubD subdiv;
    SubdivisionOptions sopts;
    sopts.levels = 2;
    bool smoothed = subdiv.subdivide(*solid, sopts);
    assert(smoothed);

    Clean cleaner;
    CleanOptions copts;
    bool cleaned = cleaner.apply(*solid, copts);
    assert(cleaned);
    assert(!solid->getMesh().getVertices().empty());
}

void testVertexToolsAndCloth()
{
    Document doc;
    Curve* base = static_cast<Curve*>(doc.geometry().addCurve(makeRectangle(1.0f, 1.0f)));
    Solid* cloth = static_cast<Solid*>(doc.geometry().extrudeCurve(base, 0.1f));
    assert(cloth);

    VertexTools vertexTools;
    std::vector<int> seeds = { 0 };
    SoftSelectionOptions softOpts;
    softOpts.radius = 1.0f;
    softOpts.translation = Vector3(0.0f, 0.2f, 0.0f);
    softOpts.rotationDegrees = 25.0f;
    softOpts.scaling = Vector3(1.0f, 1.0f, 1.1f);
    bool applied = vertexTools.applySoftTranslation(*cloth, seeds, softOpts);
    assert(applied);

    float beforeY = cloth->getMesh().getVertices().front().position.y;
    ClothEngine clothEngine;
    ClothOptions clothOpts;
    clothOpts.stiffness = 0.5f;
    clothOpts.pinnedVertices = { 0 };
    clothOpts.weightMap.resize(cloth->getMesh().getVertices().size(), 1.0f);
    Solid* colliderBase = static_cast<Solid*>(doc.geometry().extrudeCurve(static_cast<Curve*>(doc.geometry().addCurve(makeRectangle(1.0f, 1.0f))), 0.2f));
    assert(colliderBase);
    clothOpts.colliders.push_back(colliderBase);
    clothEngine.simulate(*cloth, clothOpts, 5);
    const auto& simulatedVerts = cloth->getMesh().getVertices();
    assert(simulatedVerts.front().position.y == beforeY);
    for (const auto& v : simulatedVerts) {
        assert(v.position.y >= colliderBase->getMesh().getVertices().front().position.y);
    }
}

void testCADDesigner()
{
    Document doc;
    Curve* profile = static_cast<Curve*>(doc.geometry().addCurve(makeRectangle(0.2f, 1.0f)));
    assert(profile);
    CADDesigner cad(doc.geometry());
    RevolveOptions revOpts;
    revOpts.angleDegrees = 270.0f;
    Solid* revolved = cad.revolve(*profile, revOpts);
    assert(revolved);
    ShellOptions shellOpts;
    shellOpts.thickness = 0.1f;
    Solid* shelled = cad.shell(*revolved, shellOpts);
    assert(shelled);

    std::vector<Vector3> path = {
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.5f, 0.2f },
        { 0.0f, 1.0f, 0.5f }
    };
    SweepOptions sweepOpts;
    Solid* swept = cad.sweep(*profile, path, sweepOpts);
    assert(swept);

    Solid* mirrored = cad.mirror(*revolved, Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
    assert(mirrored);

    PatternOptions patternOpts;
    patternOpts.count = 3;
    patternOpts.translationStep = Vector3(0.5f, 0.0f, 0.0f);
    auto copies = cad.pattern(*swept, patternOpts);
    assert(copies.size() == 3);

    SplitOptions splitOpts;
    splitOpts.planeNormal = Vector3(0.0f, 1.0f, 0.0f);
    splitOpts.keepPositive = true;
    Solid* sliced = cad.split(*swept, splitOpts);
    assert(sliced);

    Curve* imprint = cad.imprint(*swept, path);
    assert(imprint);
}

int main()
{
    testRoundCornerAndWeld();
    testCurveItAndPushPull();
    testSurfaceAndKnife();
    testQuadSubDAndClean();
    testVertexToolsAndCloth();
    testCADDesigner();
    return 0;
}

