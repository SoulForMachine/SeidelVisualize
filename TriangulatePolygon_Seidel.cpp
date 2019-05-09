#include "GeometryAlgorithms.h"
#include "PoolAllocator.h"
#include "geometry.h"
#include <limits>
#include <random>
#include <algorithm>
#include <numeric>
#include <cassert>


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


enum class VerticalRelation
{
	Above,
	Below
};


enum class SegmentSide
{
	Left,
	Right
};


static VerticalRelation PointsVerticalRelation(const math3d::vec2f& queryPoint, const math3d::vec2f& inRelationToPoint)
{
	// If two points have the same y coordinate, then the queryPoint is considered
	// below the inRelationToPoint if it's x coordinate is smaller.
	if (queryPoint.y < inRelationToPoint.y)
		return VerticalRelation::Below;
	else if (queryPoint.y > inRelationToPoint.y)
		return VerticalRelation::Above;
	else if (queryPoint.x < inRelationToPoint.x)
		return VerticalRelation::Below;
	else
		return VerticalRelation::Above;
}


static SegmentSide WhichSegmentSide(const math3d::vec2f& point, const Segment& segment)
{
	if (math3d::point_to_line_sgn_dist_2d(point, segment.line) > 0.0f)
		return SegmentSide::Left;
	else
		return SegmentSide::Right;
}

TrapezoidTreeState::TrapezoidTreeState(const math3d::vec2f* pts, size_t n) :
	numPoints { n },
	pointCoords { pts },
	points { },
	segments { },
	trapezoidPool { 3 * n + 1 },
	treeNodePool { (2 * n) + (3 * n + 1) },
	treeRootNode { nullptr }
{
	points.resize(n);

	for (size_t i = 0; i < n; ++i)
	{
		points[i].index = i;
		points[i].node = nullptr;
	}

	segments.resize(n);

	for (size_t i = 0; i < n; ++i)
	{
		Segment& seg = segments[i];
		size_t ptAIndex = i;
		size_t ptBIndex = (i + 1) % n;

		if (PointsVerticalRelation(pts[ptAIndex], pts[ptBIndex]) == VerticalRelation::Below)
		{
			seg.lowerPointIndex = ptAIndex;
			seg.upperPointIndex = ptBIndex;
		}
		else
		{
			seg.lowerPointIndex = ptBIndex;
			seg.upperPointIndex = ptAIndex;
		}

		seg.line = math3d::line_from_points_2d(pts[seg.lowerPointIndex], pts[seg.upperPointIndex]);
	}
}

static TrapezoidationTreeNode* AddPoint(TrapezoidTreeState& state, size_t pointIndex)
{
	// If the tree is empty, place a new vertex node as the root with two child trapezoid nodes.
	if (state.treeRootNode == nullptr)
	{
		state.treeRootNode = state.treeNodePool.New();

		// Node for lower trapezoid.
		auto leftChild = state.treeNodePool.New();
		leftChild->type = TrapezoidationTreeNode::Type::TRAPEZOID;
		leftChild->parent = state.treeRootNode;
		leftChild->trapezoid = state.trapezoidPool.New();
		leftChild->trapezoid->upperVertexIndex = static_cast<int>(pointIndex);
		leftChild->trapezoid->lowerVertexIndex = -1;
		leftChild->trapezoid->node = leftChild;

		// Node for upper trapezoid.
		auto rightChild = state.treeNodePool.New();
		rightChild->type = TrapezoidationTreeNode::Type::TRAPEZOID;
		rightChild->parent = state.treeRootNode;
		rightChild->trapezoid = state.trapezoidPool.New();
		rightChild->trapezoid->upperVertexIndex = -1;
		rightChild->trapezoid->lowerVertexIndex = static_cast<int>(pointIndex);
		rightChild->trapezoid->node = rightChild;

		leftChild->trapezoid->upper1 = rightChild->trapezoid;
		rightChild->trapezoid->lower1 = leftChild->trapezoid;

		state.treeRootNode->type = TrapezoidationTreeNode::Type::VERTEX;
		state.treeRootNode->elementIndex = pointIndex;
		state.treeRootNode->left = leftChild;
		state.treeRootNode->right = rightChild;
		state.treeRootNode->parent = nullptr;

		return state.treeRootNode;
	}

	TrapezoidationTreeNode* node = state.treeRootNode;

	while (node != nullptr)
	{
		switch (node->type)
		{
		case TrapezoidationTreeNode::Type::VERTEX:
		{
			// If this is the same vertex, return the existing node.
			if (pointIndex == node->elementIndex)
				return node;

			if (PointsVerticalRelation(state.pointCoords[pointIndex], state.pointCoords[node->elementIndex]) == VerticalRelation::Below)
			{
				node = node->left;
			}
			else // above
			{
				node = node->right;
			}
			
			break;
		}

		case TrapezoidationTreeNode::Type::SEGMENT:
		{
			if (WhichSegmentSide(state.pointCoords[pointIndex], state.segments[node->elementIndex]) == SegmentSide::Left)
			{
				node = node->left;
			}
			else // right
			{
				node = node->right;
			}

			break;
		}

		case TrapezoidationTreeNode::Type::TRAPEZOID:
		{
			// We change this trapezoid node into a vertex node.
			// We split this trapezoid in two by the horizontal line that goes through the vertex
			// and add new trapezoid nodes as children of the vertex node.

			auto upperTrapezoidNode = state.treeNodePool.New();
			auto lowerTrapezoidNode = state.treeNodePool.New();
			lowerTrapezoidNode->trapezoid = state.trapezoidPool.New();

			// The old trapezoid node will reference the upper part of the trapezoid split by the vertex
			// and become the right child of the new vertex node.
			upperTrapezoidNode->type = TrapezoidationTreeNode::Type::TRAPEZOID;
			upperTrapezoidNode->parent = node;
			upperTrapezoidNode->trapezoid = node->trapezoid;	// Reuse the trapezoid we are splitting as an upper part.
			upperTrapezoidNode->trapezoid->lowerVertexIndex = static_cast<int>(pointIndex);
			upperTrapezoidNode->trapezoid->lower1 = lowerTrapezoidNode->trapezoid;
			upperTrapezoidNode->trapezoid->lower2 = nullptr;
			upperTrapezoidNode->trapezoid->node = upperTrapezoidNode;

			// The new trapezoid node will reference the lower part of the trapezoid split by the vertex
			// and become the left child of the new vertex node.
			lowerTrapezoidNode->type = TrapezoidationTreeNode::Type::TRAPEZOID;
			lowerTrapezoidNode->parent = node;
			lowerTrapezoidNode->trapezoid->upperVertexIndex = static_cast<int>(pointIndex);
			lowerTrapezoidNode->trapezoid->lowerVertexIndex = node->trapezoid->lowerVertexIndex;
			lowerTrapezoidNode->trapezoid->upper1 = node->trapezoid;
			lowerTrapezoidNode->trapezoid->upper2 = nullptr;
			lowerTrapezoidNode->trapezoid->upper3 = nullptr;
			lowerTrapezoidNode->trapezoid->lower1 = node->trapezoid->lower1;
			lowerTrapezoidNode->trapezoid->lower2 = node->trapezoid->lower2;
			lowerTrapezoidNode->trapezoid->leftSegmentIndex = node->trapezoid->leftSegmentIndex;
			lowerTrapezoidNode->trapezoid->rightSegmentIndex = node->trapezoid->rightSegmentIndex;
			lowerTrapezoidNode->trapezoid->node = lowerTrapezoidNode;

			node->type = TrapezoidationTreeNode::Type::VERTEX;
			node->elementIndex = pointIndex;
			node->left = lowerTrapezoidNode;
			node->right = upperTrapezoidNode;

			state.points[pointIndex].node = node;

			return node;
		}
		}
	}

	// Shouldn't reach this point. If it does, the tree has a leaf node which is not of type TRAPEZOID.
	return nullptr;
}

static void ThreadSegment(
	TrapezoidTreeState& state,
	size_t segmentIndex,
	TrapezoidationTreeNode* trapNode,
	TrapezoidationTreeNode*& leftTrapNode,
	TrapezoidationTreeNode*& rightTrapNode,
	TrapezoidationTreeNode*& nextTrapNode)
{
	auto leftTrap = trapNode->trapezoid;	// Reuse the trapezoid we are splitting as a new left trapezoid.
	auto rightTrap = state.trapezoidPool.New();
	auto& segment = state.segments[segmentIndex];

	auto u1 = leftTrap->upper1;
	auto u2 = leftTrap->upper2;
	auto u3 = leftTrap->upper3;

	if (u1 != nullptr && u2 != nullptr)
	{
		// Two trapezoids above. It means the continuation of the thread.

		if (u3 != nullptr)
		{
			// There is a third upper neighbour.

			if (leftTrap->upper3Side == Trapezoid::ThirdUpperSide::LEFT)
			{
				u2->lower1 = rightTrap;
				leftTrap->upper1 = u3;
				leftTrap->upper2 = u1;
				leftTrap->upper3 = nullptr;
				rightTrap->upper1 = u2;
			}
			else
			{
				u2->lower1 = rightTrap;
				u3->lower1 = rightTrap;
				leftTrap->upper2 = nullptr;
				leftTrap->upper3 = nullptr;
				rightTrap->upper1 = u2;
				rightTrap->upper2 = u3;
			}
		}
		else
		{
			leftTrap->upper2 = nullptr;
			rightTrap->upper1 = u2;
			u2->lower1 = rightTrap;
		}
	}
	else if (u1 != nullptr)
	{
		// One trapezoid above. It will be a fresh segment or an upward cusp.

		auto ul1 = u1->lower1;
		auto ul2 = u1->lower2;

		if (ul1 != nullptr && ul2 != nullptr)
		{
			// Upward cusp. Update necessary only if this segment creates the cusp from the right side.
			if (ul1->rightSegmentIndex >= 0 &&
				WhichSegmentSide(state.pointCoords[segment.lowerPointIndex], state.segments[ul1->rightSegmentIndex]) == SegmentSide::Right)
			{
				leftTrap->upper1 = nullptr;
				rightTrap->upper1 = u1;
				u1->lower2 = rightTrap;
			}
		}
		else
		{
			// Fresh segment.
			u1->lower2 = rightTrap;
			rightTrap->upper1 = u1;
		}
	}
	else
	{
		assert(0);
	}

	auto l1 = leftTrap->lower1;
	auto l2 = leftTrap->lower2;

	if (l1 != nullptr && l2 != nullptr)
	{
		// Two trapezoids below. Find out which one is intersected by this segment.
		auto side = WhichSegmentSide(state.pointCoords[l1->upperVertexIndex], segment);

		if (side == SegmentSide::Left)
		{
			nextTrapNode = l2->node;
			rightTrap->lower1 = l2;
			l2->upper2 = rightTrap;
		}
		else
		{
			nextTrapNode = l1->node;
			leftTrap->lower2 = nullptr;
			rightTrap->lower1 = l1;
			rightTrap->lower2 = l2;
			l1->upper2 = rightTrap;
			l2->upper1 = rightTrap;
		}
	}
	else if (l1 != nullptr)
	{
		// One trapezoid below.
		nextTrapNode = l1->node;
		auto lu1 = l1->upper1;
		auto lu2 = l1->upper2;

		if (lu1 != nullptr && lu2 != nullptr)
		{
			// The trapezoid below has two upper trapezoids.

			if (segment.lowerPointIndex == l1->upperVertexIndex)
			{
				// Downward cusp. Update necessary only if this segment is creating the cusp from the right side.
			//	if (lu1->rightSegmentIndex >= 0 &&
			//		WhichSegmentSide(state.pointCoords[segment.upperPointIndex], state.segments[lu1->rightSegmentIndex]) == SegmentSide::Right)
				if (leftTrap == lu2)
				{
					leftTrap->lower1 = nullptr;
					rightTrap->lower1 = l1;
					l1->upper2 = rightTrap;
				}
			}
			else
			{
				// Threading the segment will create a third trapezoid.
				if (leftTrap == lu1)
				{
					// The trapezoid we are cutting, leftTrap, is on the left side.
					// The third trapezoid is on the right.
					l1->upper2 = rightTrap;
					l1->upper3 = lu2;
					l1->upper3Side = Trapezoid::ThirdUpperSide::RIGHT;
					rightTrap->lower1 = l1;
				}
				else
				{
					// The trapezoid we are cutting, leftTrap, is on the right side.
					// The third trapezoid is on the left.
					assert(leftTrap == lu2);
					l1->upper1 = leftTrap;
					l1->upper2 = rightTrap;
					l1->upper3 = lu1;
					l1->upper3Side = Trapezoid::ThirdUpperSide::LEFT;
					rightTrap->lower1 = l1;
				}
			}
		}
		else
		{
			// Fresh segment.
			rightTrap->lower1 = l1;
			l1->upper2 = rightTrap;
		}
	}
	else
	{
		assert(0);
	}

	// Create new left trapezoid node.
	leftTrapNode = state.treeNodePool.New();
	leftTrapNode->type = TrapezoidationTreeNode::Type::TRAPEZOID;
	leftTrapNode->parent = trapNode;
	leftTrapNode->trapezoid = leftTrap;
	leftTrapNode->trapezoid->node = leftTrapNode;
	leftTrapNode->trapezoid->rightSegmentIndex = segmentIndex;

	// Create new right trapezoid node.
	rightTrapNode = state.treeNodePool.New();
	rightTrapNode->type = TrapezoidationTreeNode::Type::TRAPEZOID;
	rightTrapNode->parent = trapNode;
	rightTrapNode->trapezoid = rightTrap;
	rightTrapNode->trapezoid->node = rightTrapNode;
	rightTrapNode->trapezoid->leftSegmentIndex = segmentIndex;

	// Change current trapezoid node into a segment node.
	trapNode->type = TrapezoidationTreeNode::Type::SEGMENT;
	trapNode->elementIndex = segmentIndex;
	trapNode->left = leftTrapNode;
	trapNode->right = rightTrapNode;
}

static TrapezoidationTreeNode* MergeTrapezoids(TrapezoidationTreeNode* prevTrapNode, TrapezoidationTreeNode* curTrapNode)
{
	return curTrapNode;
};

static void AddSegment(TrapezoidTreeState& state, size_t segmentIndex)
{
	Segment& segment = state.segments[segmentIndex];

	// First add upper and lower segment vertices to the tree.

	if (state.points[segment.upperPointIndex].node == nullptr)
		AddPoint(state, segment.upperPointIndex);

	if (state.points[segment.lowerPointIndex].node == nullptr)
		AddPoint(state, segment.lowerPointIndex);

	// Thread the segment from its upper point to its lower point through trapezoids and split
	// them in half.

	TrapezoidationTreeNode* trapezoidNode = state.points[segment.upperPointIndex].node->left;
	TrapezoidationTreeNode* prevLeftTrapNode = nullptr;
	TrapezoidationTreeNode* prevRightTrapNode = nullptr;

	while (trapezoidNode->trapezoid->upperVertexIndex != segment.lowerPointIndex)
	{
		TrapezoidationTreeNode* leftTrapezoidNode;
		TrapezoidationTreeNode* rightTrapezoidNode;
		TrapezoidationTreeNode* nextTrapezoidNode;

		ThreadSegment(state, segmentIndex, trapezoidNode, leftTrapezoidNode, rightTrapezoidNode, nextTrapezoidNode);

		prevLeftTrapNode = MergeTrapezoids(prevLeftTrapNode, leftTrapezoidNode);
		prevRightTrapNode = MergeTrapezoids(prevRightTrapNode, rightTrapezoidNode);

		trapezoidNode = nextTrapezoidNode;
	}
}

static void BuildTrapezoidTree(TrapezoidTreeState& state)
{
	// Generate random sequence of line segments.
	std::vector<size_t> segmentIndices(state.numPoints);
	std::iota(segmentIndices.begin(), segmentIndices.end(), 0);
	std::shuffle(segmentIndices.begin(), segmentIndices.end(), std::mt19937 { });

	// Add each segment to the tree.
	for (size_t segInd : segmentIndices)
		AddSegment(state, segInd);
}

static void BuildYMonotoneChains()
{

}

static void DoEarClipping()
{

}


bool TriangulatePolygon_Seidel(const math3d::vec2f* points, size_t numPoints, std::vector<unsigned short>& outIndices)
{
	if (points == nullptr ||
		numPoints < 3 ||
		numPoints > std::numeric_limits<unsigned short>::max())
	{
		return false;
	}

	const size_t numDiagonals = numPoints - 3;
	const size_t numTriangles = numPoints - 2;
	const size_t maxNumTrapezoids = 3 * numPoints + 1;

	TrapezoidTreeState state { points, numPoints };

	BuildTrapezoidTree(state);
	BuildYMonotoneChains();
	DoEarClipping();

	return true;
}

} // namespace BaseLib::Geometry
