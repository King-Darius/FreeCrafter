#include "SceneSettings.h"

#include <algorithm>
#include <istream>
#include <ostream>
#include <sstream>

namespace {

Scene::SceneSettings::PaletteState defaultPalette()
{
    Scene::SceneSettings::PaletteState state;
    state.id = "softGreen";
    state.fill = {0.74f, 0.83f, 0.78f, 1.0f};
    state.edge = {0.20f, 0.30f, 0.24f, 1.0f};
    state.highlight = {0.36f, 0.62f, 0.49f, 1.0f};
    return state;
}

Scene::SceneSettings::Color clampColor(const Scene::SceneSettings::Color& color)
{
    Scene::SceneSettings::Color clamped = color;
    clamped.r = std::clamp(clamped.r, 0.0f, 1.0f);
    clamped.g = std::clamp(clamped.g, 0.0f, 1.0f);
    clamped.b = std::clamp(clamped.b, 0.0f, 1.0f);
    clamped.a = std::clamp(clamped.a, 0.0f, 1.0f);
    return clamped;
}

} // namespace

namespace Scene {

SceneSettings::SceneSettings()
{
    reset();
}

void SceneSettings::reset()
{
    planesVisible = true;
    fillsVisible = true;
    guidesVisibleFlag = true;
    gridSettings = {};
    shadowSettings = {};
    paletteState = defaultPalette();
}

void SceneSettings::setPalette(const PaletteState& state)
{
    paletteState.id = state.id;
    paletteState.fill = clampColor(state.fill);
    paletteState.edge = clampColor(state.edge);
    paletteState.highlight = clampColor(state.highlight);
}

void SceneSettings::serialize(std::ostream& os) const
{
    os << "planes " << (planesVisible ? 1 : 0) << '\n';
    os << "fills " << (fillsVisible ? 1 : 0) << '\n';
    os << "guides " << (guidesVisibleFlag ? 1 : 0) << '\n';
    os << "gridMajorSpacing " << gridSettings.majorSpacing << '\n';
    os << "gridMinorDivisions " << gridSettings.minorDivisions << '\n';
    os << "gridMajorExtent " << gridSettings.majorExtent << '\n';
    os << "shadowEnabled " << (shadowSettings.enabled ? 1 : 0) << '\n';
    os << "shadowQuality " << static_cast<int>(shadowSettings.quality) << '\n';
    os << "shadowStrength " << shadowSettings.strength << '\n';
    os << "shadowBias " << shadowSettings.bias << '\n';
    os << "palette " << paletteState.id << '\n';
    os << "fill " << paletteState.fill.r << ' ' << paletteState.fill.g << ' ' << paletteState.fill.b << ' '
       << paletteState.fill.a << '\n';
    os << "edge " << paletteState.edge.r << ' ' << paletteState.edge.g << ' ' << paletteState.edge.b << ' '
       << paletteState.edge.a << '\n';
    os << "highlight " << paletteState.highlight.r << ' ' << paletteState.highlight.g << ' '
       << paletteState.highlight.b << ' ' << paletteState.highlight.a << '\n';
}

bool SceneSettings::deserialize(std::istream& is, int version)
{
    reset();
    if (version < 3) {
        int planes = 1;
        int fills = 1;
        if (!(is >> planes >> fills)) {
            return false;
        }
        planesVisible = planes != 0;
        fillsVisible = fills != 0;
        return true;
    }

    std::string line;
    while (std::getline(is, line)) {
        if (line.empty())
            continue;
        if (line == "END_SETTINGS")
            break;
        std::istringstream ss(line);
        std::string key;
        ss >> key;
        if (key == "planes") {
            int value = planesVisible ? 1 : 0;
            if (ss >> value)
                planesVisible = value != 0;
        } else if (key == "fills") {
            int value = fillsVisible ? 1 : 0;
            if (ss >> value)
                fillsVisible = value != 0;
        } else if (key == "guides") {
            int value = guidesVisibleFlag ? 1 : 0;
            if (ss >> value)
                guidesVisibleFlag = value != 0;
        } else if (key == "gridMajorSpacing") {
            ss >> gridSettings.majorSpacing;
            gridSettings.majorSpacing = std::max(0.001f, gridSettings.majorSpacing);
        } else if (key == "gridMinorDivisions") {
            ss >> gridSettings.minorDivisions;
            gridSettings.minorDivisions = std::max(1, gridSettings.minorDivisions);
        } else if (key == "gridMajorExtent") {
            ss >> gridSettings.majorExtent;
            gridSettings.majorExtent = std::max(1, gridSettings.majorExtent);
        } else if (key == "shadowEnabled") {
            int value = shadowSettings.enabled ? 1 : 0;
            if (ss >> value)
                shadowSettings.enabled = value != 0;
        } else if (key == "shadowQuality") {
            int quality = static_cast<int>(shadowSettings.quality);
            if (ss >> quality) {
                quality = std::clamp(quality, 0, 2);
                shadowSettings.quality = static_cast<ShadowQuality>(quality);
            }
        } else if (key == "shadowStrength") {
            ss >> shadowSettings.strength;
            shadowSettings.strength = std::clamp(shadowSettings.strength, 0.0f, 1.0f);
        } else if (key == "shadowBias") {
            ss >> shadowSettings.bias;
            shadowSettings.bias = std::max(0.00005f, shadowSettings.bias);
        } else if (key == "palette") {
            ss >> paletteState.id;
        } else if (key == "fill") {
            ss >> paletteState.fill.r >> paletteState.fill.g >> paletteState.fill.b >> paletteState.fill.a;
        } else if (key == "edge") {
            ss >> paletteState.edge.r >> paletteState.edge.g >> paletteState.edge.b >> paletteState.edge.a;
        } else if (key == "highlight") {
            ss >> paletteState.highlight.r >> paletteState.highlight.g >> paletteState.highlight.b
               >> paletteState.highlight.a;
        }
    }

    if (paletteState.id.empty())
        paletteState = defaultPalette();

    return true;
}

} // namespace Scene
