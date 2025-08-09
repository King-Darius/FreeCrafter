#pragma once
#include <memory>
#include <iosfwd>

class Curve;
class Solid;

namespace GeometryIO {
void writeCurve(std::ostream& os, const Curve& curve);
std::unique_ptr<Curve> readCurve(std::istream& is);

void writeSolid(std::ostream& os, const Solid& solid);
std::unique_ptr<Solid> readSolid(std::istream& is);
}
