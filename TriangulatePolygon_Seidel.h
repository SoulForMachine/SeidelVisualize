#ifndef _BASE_LIB_INTERFACE_GEOMETRY_ALGORITHMS_H_
#define _BASE_LIB_INTERFACE_GEOMETRY_ALGORITHMS_H_

#include <vector>
#include <random>
#include "vec2.h"


using int_t = std::intmax_t;
using index_t = std::ptrdiff_t;

namespace Geometry
{


struct TrapezoidationTreeNode;


struct Point
{
	index_t index = 0;
	
	// 1-based indices of segments that share this point.
	// If the point is left point of the segment, the index is positive.
	// If it is right point of the segment, the index is negative.
	index_t seg1Index = 0;
	index_t seg2Index = 0;

	TrapezoidationTreeNode* node = nullptr;
};


struct Segment
{
	index_t upperPointIndex;
	index_t lowerPointIndex;
	math3d::vec3f line;
	bool upward;
};


struct Trapezoid
{
	Trapezoid()
		: number { nextNumber++ }
	{ }

	enum class ThirdUpperSide
	{
		LEFT,
		RIGHT
	};

	index_t upperPointIndex = -1;
	index_t lowerPointIndex = -1;
	Trapezoid* upper1 = nullptr;
	Trapezoid* upper2 = nullptr;
	Trapezoid* upper3 = nullptr;
	Trapezoid* lower1 = nullptr;
	Trapezoid* lower2 = nullptr;
	ThirdUpperSide upper3Side = ThirdUpperSide::LEFT;
	index_t leftSegmentIndex = -1;
	index_t rightSegmentIndex = -1;
	TrapezoidationTreeNode* node = nullptr;
	bool inside = false;
	bool visited[2] = { false, false };
	bool hasDiagonal = false;

	const int_t number;
	static int_t nextNumber;
};


struct TrapezoidationTreeNode
{
	enum class Type
	{
		POINT,
		SEGMENT,
		TRAPEZOID
	};

	Type type;

	union
	{
		index_t elementIndex;
		Trapezoid* trapezoid;
	};

	TrapezoidationTreeNode* left = nullptr;
	TrapezoidationTreeNode* right = nullptr;
	TrapezoidationTreeNode* parent = nullptr;

	int_t leftWeight = 0;
	int_t rightWeight = 0;
};


enum class Winding
{
	CW,
	CCW,
};

enum class FillRule
{
	NON_ZERO,
	ODD,
};

struct TriangulationState
{
	TriangulationState(const std::vector<std::vector<math3d::vec2f>>& outlines);
	~TriangulationState();

	std::vector<math3d::vec2f> pointCoords;
	std::vector<Point> points;
	std::vector<Segment> segments;
	std::vector<TrapezoidationTreeNode*> treeNodes;
	std::vector<Trapezoid*> trapezoids;
	std::vector<unsigned short> outIndices;
	std::vector<std::vector<index_t>> monChains;

	TrapezoidationTreeNode* treeRootNode = nullptr;
	std::mt19937 rndEng { std::random_device{}() };
	bool randomizeSegments = false;
	Winding triangleWinding = Winding::CCW;
	FillRule fillRule = FillRule::ODD;

	int_t dbgSteps = std::numeric_limits<int_t>::max();
};


//
// Triangulates one or more closed polygonal curves with no intersecting segments using Seidel's algorithm.
// Input: points of simple closed polygonal curves.
// For each outline points[i] and points[(i + 1) % numPoints] are endpoints of one line segment.
// It is assumed that there are no equal points.
// Output: an array of index triplets which form triangles by indexing into the input array of points.
//
bool TriangulatePolygon_Seidel(TriangulationState& state);

} // namespace BaseLib::Geometry

#endif // _BASE_LIB_INTERFACE_GEOMETRY_ALGORITHMS_H_
