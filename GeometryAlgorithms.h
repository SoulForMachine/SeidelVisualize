#ifndef _BASE_LIB_INTERFACE_GEOMETRY_ALGORITHMS_H_
#define _BASE_LIB_INTERFACE_GEOMETRY_ALGORITHMS_H_

#include <vector>
#include "vec2.h"


namespace Geometry
{

//
// Triangulates a closed polygonal curve with no intersecting segments using Seidel's algorithm.
// Input: points of a simple closed polygonal curve.
// points[i] and points[(i + 1) % numPoints] are endpoints of one line segment.
// It is assumed that there are no equal points.
// Output: an array of index triplets which form numPoints - 2 triangles by indexing into the input array of points.
//
bool TriangulatePolygon_Seidel(const math3d::vec2f* points, size_t numPoints, std::vector<unsigned short>& outIndices);

} // namespace BaseLib::Geometry

#endif // _BASE_LIB_INTERFACE_GEOMETRY_ALGORITHMS_H_
