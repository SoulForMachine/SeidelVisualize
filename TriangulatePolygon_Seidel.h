#ifndef _BASE_LIB_INTERFACE_GEOMETRY_ALGORITHMS_H_
#define _BASE_LIB_INTERFACE_GEOMETRY_ALGORITHMS_H_

#include <vector>
#include "vec2.h"
#include "PoolAllocator.h"


namespace Geometry
{


struct TrapezoidationTreeNode;


struct Point
{
	size_t index;
	TrapezoidationTreeNode* node;
};


struct Segment
{
	size_t upperPointIndex;
	size_t lowerPointIndex;
	math3d::vec3f line;
};


struct Trapezoid
{
	enum class ThirdUpperSide
	{
		LEFT,
		RIGHT
	};

	enum class Status
	{
		Inside,
		Outside
	};

	int upperVertexIndex = -1;
	int lowerVertexIndex = -1;
	Trapezoid* upper1 = nullptr;
	Trapezoid* upper2 = nullptr;
	Trapezoid* upper3 = nullptr;
	Trapezoid* lower1 = nullptr;
	Trapezoid* lower2 = nullptr;
	ThirdUpperSide upper3Side = ThirdUpperSide::LEFT;
	int leftSegmentIndex = -1;
	int rightSegmentIndex = -1;
	TrapezoidationTreeNode* node = nullptr;
	Status state = Status::Outside;
};


struct TrapezoidationTreeNode
{
	enum class Type
	{
		VERTEX,
		SEGMENT,
		TRAPEZOID
	};

	Type type;

	union
	{
		size_t elementIndex;
		Trapezoid* trapezoid;
	};

	TrapezoidationTreeNode* left = nullptr;
	TrapezoidationTreeNode* right = nullptr;
	TrapezoidationTreeNode* parent = nullptr;

	unsigned int leftWeight = 0;
	unsigned int rightWeight = 0;
};


struct TrapezoidTreeState
{
	TrapezoidTreeState(const math3d::vec2f* pts, size_t n);

	size_t numPoints;
	const math3d::vec2f* pointCoords;
	std::vector<Point> points;
	std::vector<Segment> segments;
	BaseLib::PoolAllocator<Trapezoid> trapezoidPool;
	BaseLib::PoolAllocator<TrapezoidationTreeNode> treeNodePool;
	TrapezoidationTreeNode* treeRootNode;
};


//
// Triangulates a closed polygonal curve with no intersecting segments using Seidel's algorithm.
// Input: points of a simple closed polygonal curve.
// points[i] and points[(i + 1) % numPoints] are endpoints of one line segment.
// It is assumed that there are no equal points.
// Output: an array of index triplets which form numPoints - 2 triangles by indexing into the input array of points.
//
bool TriangulatePolygon_Seidel(TrapezoidTreeState& state, std::vector<unsigned short>& outIndices);

} // namespace BaseLib::Geometry

#endif // _BASE_LIB_INTERFACE_GEOMETRY_ALGORITHMS_H_
