#include "TriangulatePolygon_Seidel.h"
#include "geometry.h"
#include <limits>
#include <algorithm>
#include <numeric>
#include <cassert>


namespace Geometry
{

int Trapezoid::nextNumber = 1;


enum class VerticalRelation
{
	ABOVE,
	BELOW
};


enum class Side
{
	LEFT,
	RIGHT
};


static VerticalRelation PointsVerticalRelation(const math3d::vec2f& queryPoint, const math3d::vec2f& inRelationToPoint)
{
	// If two points have the same y coordinate, then the queryPoint is considered
	// below the inRelationToPoint if it's x coordinate is smaller.
	if (queryPoint.y < inRelationToPoint.y)
		return VerticalRelation::BELOW;
	else if (queryPoint.y > inRelationToPoint.y)
		return VerticalRelation::ABOVE;
	else if (queryPoint.x < inRelationToPoint.x)
		return VerticalRelation::BELOW;
	else
		return VerticalRelation::ABOVE;
}


static Side WhichSegmentSide(const math3d::vec2f& point, const Segment& segment)
{
	if (math3d::point_to_line_sgn_dist_2d(point, segment.line) > 0.0f)
		return Side::LEFT;
	else
		return Side::RIGHT;
}

TriangulationState::TriangulationState(const std::vector<std::vector<math3d::vec2f>>& outlines)
{
	size_t numPoints = 0;
	size_t numTris = 0;
	for (auto& outl : outlines)
	{
		pointCoords.insert(pointCoords.end(), outl.begin(), outl.end());
		numPoints += outl.size();
		numTris += outl.size() - 2;
	}

	points.resize(numPoints);

	for (size_t i = 0; i < numPoints; ++i)
	{
		points[i].index = i;
		points[i].node = nullptr;
	}

	segments.resize(numPoints);

	size_t i = 0;
	for (auto& outl : outlines)
	{
		for (size_t j = 0; j < outl.size(); ++j)
		{
			Segment& seg = segments[i + j];
			size_t ptAIndex = j;
			size_t ptBIndex = (j + 1) % outl.size();

			if (PointsVerticalRelation(outl[ptAIndex], outl[ptBIndex]) == VerticalRelation::BELOW)
			{
				seg.lowerPointIndex = i + ptAIndex;
				seg.upperPointIndex = i + ptBIndex;
				seg.upward = true;
			}
			else
			{
				seg.lowerPointIndex = i + ptBIndex;
				seg.upperPointIndex = i + ptAIndex;
				seg.upward = false;
			}

			seg.line = math3d::line_from_points_2d(pointCoords[seg.lowerPointIndex], pointCoords[seg.upperPointIndex]);
		}

		i += outl.size();
	}
}

TriangulationState::~TriangulationState()
{
	for (auto trap : trapezoids)
		delete trap;

	for (auto node : treeNodes)
		delete node;
}

static Trapezoid* AllocateTrapezoid(TriangulationState& state)
{
	auto newTrap = new Trapezoid;
	state.trapezoids.push_back(newTrap);
	return newTrap;
}

static void DeallocateTrapezoid(TriangulationState& state, Trapezoid* trapezoid)
{
	for (auto it = state.trapezoids.begin(); it != state.trapezoids.end(); ++it)
	{
		if (*it == trapezoid)
		{
			state.trapezoids.erase(it);
			break;
		}
	}

	delete trapezoid;
}

static TrapezoidationTreeNode* AllocateTrapTreeNode(TriangulationState& state)
{
	auto newNode = new TrapezoidationTreeNode;
	state.treeNodes.push_back(newNode);
	return newNode;
}

static void DeallocateTrapTreeNode(TriangulationState& state, TrapezoidationTreeNode* node)
{
	for (auto it = state.treeNodes.begin(); it != state.treeNodes.end(); ++it)
	{
		if (*it == node)
		{
			state.treeNodes.erase(it);
			break;
		}
	}

	delete node;
}

// Add the point to the tree and return a pointer to the point node.
// If the point has already been inserted, return a pointer to an existing point node.
static TrapezoidationTreeNode* AddPoint(TriangulationState& state, size_t pointIndex)
{
	// If the tree is empty, place a new vertex node as the root with two child trapezoid nodes.
	if (state.treeRootNode == nullptr)
	{
		auto rightChild = AllocateTrapTreeNode(state);
		auto leftChild = AllocateTrapTreeNode(state);

		state.treeRootNode = AllocateTrapTreeNode(state);
		state.treeRootNode->type = TrapezoidationTreeNode::Type::POINT;
		state.treeRootNode->elementIndex = pointIndex;
		state.treeRootNode->left = leftChild;
		state.treeRootNode->right = rightChild;

		// Node for upper trapezoid.
		rightChild->type = TrapezoidationTreeNode::Type::TRAPEZOID;
		rightChild->parent = state.treeRootNode;
		rightChild->trapezoid = AllocateTrapezoid(state);
		rightChild->trapezoid->lowerPointIndex = static_cast<int>(pointIndex);
		rightChild->trapezoid->node = rightChild;

		// Node for lower trapezoid.
		leftChild->type = TrapezoidationTreeNode::Type::TRAPEZOID;
		leftChild->parent = state.treeRootNode;
		leftChild->trapezoid = AllocateTrapezoid(state);
		leftChild->trapezoid->upperPointIndex = static_cast<int>(pointIndex);
		leftChild->trapezoid->node = leftChild;

		leftChild->trapezoid->upper1 = rightChild->trapezoid;
		rightChild->trapezoid->lower1 = leftChild->trapezoid;

		state.points[pointIndex].node = state.treeRootNode;

		return state.treeRootNode;
	}

	TrapezoidationTreeNode* node = state.treeRootNode;

	while (node != nullptr)
	{
		switch (node->type)
		{
		case TrapezoidationTreeNode::Type::POINT:
		{
			// If this is the same vertex, return the existing node.
			if (pointIndex == node->elementIndex)
				return node;

			auto rel = PointsVerticalRelation(state.pointCoords[pointIndex], state.pointCoords[node->elementIndex]);
			node = (rel == VerticalRelation::BELOW) ? node->left : node->right;
			break;
		}

		case TrapezoidationTreeNode::Type::SEGMENT:
		{
			auto side = WhichSegmentSide(state.pointCoords[pointIndex], state.segments[node->elementIndex]);
			node = (side == Side::LEFT) ? node->left : node->right;
			break;
		}

		case TrapezoidationTreeNode::Type::TRAPEZOID:
		{
			// We change this trapezoid node into a vertex node.
			// We split this trapezoid in two by the horizontal line that goes through the vertex
			// and add new trapezoid nodes as children of the vertex node.

			auto upperTrapezoidNode = AllocateTrapTreeNode(state);
			auto lowerTrapezoidNode = AllocateTrapTreeNode(state);

			auto oldTrap = node->trapezoid;
			auto newTrap = AllocateTrapezoid(state);

			// The new trapezoid node will reference the lower part of the trapezoid split by the vertex
			// and become the left child of the new vertex node.
			lowerTrapezoidNode->type = TrapezoidationTreeNode::Type::TRAPEZOID;
			lowerTrapezoidNode->parent = node;
			lowerTrapezoidNode->trapezoid = newTrap;
			lowerTrapezoidNode->trapezoid->upperPointIndex = static_cast<int>(pointIndex);
			lowerTrapezoidNode->trapezoid->lowerPointIndex = oldTrap->lowerPointIndex;
			lowerTrapezoidNode->trapezoid->upper1 = oldTrap;
			lowerTrapezoidNode->trapezoid->lower1 = oldTrap->lower1;
			lowerTrapezoidNode->trapezoid->lower2 = oldTrap->lower2;
			lowerTrapezoidNode->trapezoid->leftSegmentIndex = oldTrap->leftSegmentIndex;
			lowerTrapezoidNode->trapezoid->rightSegmentIndex = oldTrap->rightSegmentIndex;
			lowerTrapezoidNode->trapezoid->node = lowerTrapezoidNode;

			// For trapezoids below the old one, set the new trapezoid as their upper neighbour.
			if (oldTrap->lower1 != nullptr)
			{
				if (oldTrap->lower1->upper1 == oldTrap)
					oldTrap->lower1->upper1 = newTrap;
				else if (oldTrap->lower1->upper2 == oldTrap)
					oldTrap->lower1->upper2 = newTrap;

				assert(oldTrap->lower1->upper3 == nullptr);
			}

			if (oldTrap->lower2 != nullptr)
			{
				if (oldTrap->lower2->upper1 == oldTrap)
					oldTrap->lower2->upper1 = newTrap;
				else if (oldTrap->lower2->upper2 == oldTrap)
					oldTrap->lower2->upper2 = newTrap;
			
				assert(oldTrap->lower2->upper3 == nullptr);
			}

			// The old trapezoid node will reference the upper part of the trapezoid split by the vertex
			// and become the right child of the new vertex node.
			upperTrapezoidNode->type = TrapezoidationTreeNode::Type::TRAPEZOID;
			upperTrapezoidNode->parent = node;
			upperTrapezoidNode->trapezoid = oldTrap;	// Reuse the trapezoid we are splitting as an upper part.
			upperTrapezoidNode->trapezoid->lowerPointIndex = static_cast<int>(pointIndex);
			upperTrapezoidNode->trapezoid->lower1 = lowerTrapezoidNode->trapezoid;
			upperTrapezoidNode->trapezoid->lower2 = nullptr;
			upperTrapezoidNode->trapezoid->node = upperTrapezoidNode;

			node->type = TrapezoidationTreeNode::Type::POINT;
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
	TriangulationState& state,
	size_t segmentIndex,
	TrapezoidationTreeNode* trapNode,
	TrapezoidationTreeNode*& leftTrapNode,
	TrapezoidationTreeNode*& rightTrapNode,
	TrapezoidationTreeNode*& nextTrapNode)
{
	auto leftTrap = trapNode->trapezoid;	// Reuse the trapezoid we are splitting as a new left trapezoid.
	auto rightTrap = AllocateTrapezoid(state);
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
				WhichSegmentSide(state.pointCoords[segment.lowerPointIndex], state.segments[ul1->rightSegmentIndex]) == Side::RIGHT)
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
		assert(l1->upperPointIndex == l2->upperPointIndex);

		// Two trapezoids below.
		if (segment.lowerPointIndex == l1->upperPointIndex)
		{
			// This segment connects with the segment below.
			nextTrapNode = l1->node;	// Either one will do; segment threading ends here.
			leftTrap->lower2 = nullptr;
			rightTrap->lower1 = l2;
			l2->upper1 = rightTrap;
		}
		else
		{
			// Find out which trapezoid below is intersected by this segment.
			auto side = WhichSegmentSide(state.pointCoords[l1->upperPointIndex], segment);

			if (side == Side::LEFT)
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

			if (segment.lowerPointIndex == l1->upperPointIndex)
			{
				// Downward cusp. Update necessary only if this segment is creating the cusp from the right side.
				if (lu1->rightSegmentIndex >= 0 &&
					WhichSegmentSide(state.pointCoords[segment.upperPointIndex], state.segments[lu1->rightSegmentIndex]) == Side::RIGHT)
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

	int rightSegIndex = leftTrap->rightSegmentIndex;

	// Create new left trapezoid node.
	leftTrapNode = AllocateTrapTreeNode(state);
	leftTrapNode->type = TrapezoidationTreeNode::Type::TRAPEZOID;
	leftTrapNode->parent = trapNode;
	leftTrapNode->trapezoid = leftTrap;
	leftTrapNode->trapezoid->node = leftTrapNode;
	leftTrapNode->trapezoid->rightSegmentIndex = segmentIndex;

	// Create new right trapezoid node.
	rightTrapNode = AllocateTrapTreeNode(state);
	rightTrapNode->type = TrapezoidationTreeNode::Type::TRAPEZOID;
	rightTrapNode->parent = trapNode;
	rightTrapNode->trapezoid = rightTrap;
	rightTrapNode->trapezoid->node = rightTrapNode;
	rightTrapNode->trapezoid->leftSegmentIndex = segmentIndex;
	rightTrapNode->trapezoid->rightSegmentIndex = rightSegIndex;
	rightTrapNode->trapezoid->upperPointIndex = leftTrap->upperPointIndex;
	rightTrapNode->trapezoid->lowerPointIndex = leftTrap->lowerPointIndex;

	// Change current trapezoid node into a segment node.
	trapNode->type = TrapezoidationTreeNode::Type::SEGMENT;
	trapNode->elementIndex = segmentIndex;
	trapNode->left = leftTrapNode;
	trapNode->right = rightTrapNode;
}

static TrapezoidationTreeNode* GetFirstTrapezoidForNewSegment(TriangulationState& state, const Segment& segment)
{
	TrapezoidationTreeNode* node = state.treeRootNode;

	while (node != nullptr)
	{
		switch (node->type)
		{
		case TrapezoidationTreeNode::Type::POINT:
		{
			// We found the upper vertex node, now find the first trapezoid node towards the lower point.
			if (segment.upperPointIndex == node->elementIndex)
			{
				// We continue search below the vertex.
				node = node->left;

				while (node != nullptr)
				{
					switch (node->type)
					{
					case TrapezoidationTreeNode::Type::POINT:
					{
						// Since this vertex is below upper vertex, the trapezoid must be above it.
						node = node->right;
						break;
					}

					case TrapezoidationTreeNode::Type::TRAPEZOID:
					{
						return node;
					}

					case TrapezoidationTreeNode::Type::SEGMENT:
					{
						size_t ptIndex;

						// Procede to the side on which the other point is.
						if (state.segments[node->elementIndex].lowerPointIndex == segment.upperPointIndex ||
							state.segments[node->elementIndex].upperPointIndex == segment.upperPointIndex)
						{
							ptIndex = segment.lowerPointIndex;
						}
						else if (state.segments[node->elementIndex].lowerPointIndex == segment.lowerPointIndex ||
								 state.segments[node->elementIndex].upperPointIndex == segment.lowerPointIndex)
						{
							ptIndex = segment.upperPointIndex;
						}
						else
						{
						//	assert(false);
							ptIndex = segment.upperPointIndex;
						}

						auto side = WhichSegmentSide(state.pointCoords[ptIndex], state.segments[node->elementIndex]);
						node = (side == Side::LEFT) ? node->left : node->right;
						break;
					}
					}
				}
			}
			else
			{
				auto rel = PointsVerticalRelation(state.pointCoords[segment.upperPointIndex], state.pointCoords[node->elementIndex]);
				node = (rel == VerticalRelation::BELOW) ? node->left : node->right;
			}

			break;
		}

		case TrapezoidationTreeNode::Type::SEGMENT:
		{
			auto side = WhichSegmentSide(state.pointCoords[segment.upperPointIndex], state.segments[node->elementIndex]);
			node = (side == Side::LEFT) ? node->left : node->right;
			break;
		}

		case TrapezoidationTreeNode::Type::TRAPEZOID:
		{
			// The upper point is not inserted, this is the trapezoid where it should be.
			return node;
		}
		}
	}

	return nullptr;
}

static TrapezoidationTreeNode* MergeTrapezoids(TriangulationState& state, TrapezoidationTreeNode* prevTrapNode, TrapezoidationTreeNode* curTrapNode)
{
	if (prevTrapNode == nullptr)
		return curTrapNode;

	auto prevTrap = prevTrapNode->trapezoid;
	auto curTrap = curTrapNode->trapezoid;

	if (prevTrap->leftSegmentIndex == curTrap->leftSegmentIndex &&
		prevTrap->rightSegmentIndex == curTrap->rightSegmentIndex)
	{
		auto l1 = curTrap->lower1;
		auto l2 = curTrap->lower2;

		prevTrap->lower1 = l1;
		prevTrap->lower2 = l2;
		prevTrap->lowerPointIndex = curTrap->lowerPointIndex;

		if (l1 != nullptr)
		{
			if (l1->upper1 == curTrap)
				l1->upper1 = prevTrap;
			else if (l1->upper2 == curTrap)
				l1->upper2 = prevTrap;
			else if (l1->upper3 == curTrap)
				l1->upper3 = prevTrap;
		}

		if (l2 != nullptr)
		{
			if (l2->upper1 == curTrap)
				l2->upper1 = prevTrap;
			else if (l2->upper2 == curTrap)
				l2->upper2 = prevTrap;
			else if (l2->upper3 == curTrap)
				l2->upper3 = prevTrap;
		}

		if (curTrapNode->parent->left == curTrapNode)
			curTrapNode->parent->left = prevTrapNode;
		else if (curTrapNode->parent->right == curTrapNode)
			curTrapNode->parent->right = prevTrapNode;

		DeallocateTrapezoid(state, curTrap);
		DeallocateTrapTreeNode(state, curTrapNode);
		return prevTrapNode;
	}

	return curTrapNode;
}

static void AddSegment(TriangulationState& state, size_t segmentIndex)
{
	Segment& segment = state.segments[segmentIndex];

	// First add upper and lower segment vertices to the tree.

	if (state.points[segment.upperPointIndex].node == nullptr)
		AddPoint(state, segment.upperPointIndex);

	state.dbgSteps--;
	if (state.dbgSteps == 0)
		return;

	if (state.points[segment.lowerPointIndex].node == nullptr)
		AddPoint(state, segment.lowerPointIndex);

	state.dbgSteps--;
	if (state.dbgSteps == 0)
		return;

	// Thread the segment from its upper point to its lower point through trapezoids and split
	// them in half.

	TrapezoidationTreeNode* trapezoidNode = GetFirstTrapezoidForNewSegment(state, segment);
	TrapezoidationTreeNode* prevLeftTrapNode = nullptr;
	TrapezoidationTreeNode* prevRightTrapNode = nullptr;

	assert(trapezoidNode != nullptr);

	while (trapezoidNode->trapezoid->upperPointIndex != segment.lowerPointIndex)
	{
		TrapezoidationTreeNode* leftTrapezoidNode;
		TrapezoidationTreeNode* rightTrapezoidNode;
		TrapezoidationTreeNode* nextTrapezoidNode;

		ThreadSegment(state, segmentIndex, trapezoidNode, leftTrapezoidNode, rightTrapezoidNode, nextTrapezoidNode);

		// After the split, merge left and right trapezoids with their upper neighbours if they are bounded by
		// the same left and right segments.
		prevLeftTrapNode = MergeTrapezoids(state, prevLeftTrapNode, leftTrapezoidNode);
		prevRightTrapNode = MergeTrapezoids(state, prevRightTrapNode, rightTrapezoidNode);

		trapezoidNode = nextTrapezoidNode;

		state.dbgSteps--;
		if (state.dbgSteps == 0)
			return;
	}
}

static void DetermineInsideTrapezoids(TriangulationState& state)
{
	auto countCrossings = [&state](int segIndex, int& counter) {
		auto& segment = state.segments[segIndex];

		if (segment.upward)
			counter--;
		else
			counter++;
	};

	for (auto trap : state.trapezoids)
	{
		if (trap->lowerPointIndex < 0 ||
			trap->upperPointIndex < 0 ||
			trap->leftSegmentIndex < 0 ||
			trap->rightSegmentIndex < 0)
		{
			continue;
		}

		auto node = trap->node;
		Side direction = Side::LEFT;
		int segmentCrossCounter = 0;

		// Traverse upwards until the left or right segment of this trapezoid is encountered.
		// The segment side determines the direction.
		while (true)
		{
			node = node->parent;

			if (node == state.treeRootNode)
			{
				assert(false);
				return;
			}

			if (node->type == TrapezoidationTreeNode::Type::SEGMENT)
			{
				if (node->elementIndex == trap->leftSegmentIndex)
				{
					countCrossings(trap->leftSegmentIndex, segmentCrossCounter);
					direction = Side::LEFT;
					break;
				}
				else if (node->elementIndex == trap->rightSegmentIndex)
				{
					countCrossings(trap->rightSegmentIndex, segmentCrossCounter);
					direction = Side::RIGHT;
					break;
				}
			}
		}

		int pointCount = 0;
		bool finished = false;
		node = (direction == Side::LEFT) ? node->left : node->right;

		// From the segment node, traverse the tree downwards to left or right,
		// depending on direction that was determined, until a trapezoid node
		// is reached. This will be an adjacent trapezoid to the one we started
		// with. Procede until a trapezoid without left or right segment is encountered.
		while (!finished)
		{
			if (node == nullptr)
			{
				assert(false);
				return;
			}

			switch (node->type)
			{
			case TrapezoidationTreeNode::Type::POINT:
			{
				pointCount++;
				node = (pointCount % 2 == 1) ? node->left : node->right;
				break;
			}

			case TrapezoidationTreeNode::Type::SEGMENT:
			{
				node = (direction == Side::LEFT) ? node->right : node->left;
				break;
			}

			case TrapezoidationTreeNode::Type::TRAPEZOID:
			{
				// We have reached an adjacent trapezoid.
				pointCount = 0;
				auto adjTrap = node->trapezoid;

				if (adjTrap->leftSegmentIndex >= 0 && adjTrap->rightSegmentIndex >= 0)
				{
					// This trapezoid has both left and right segments. Traverse upwards until the segment that matches
					// current direction is reached.
					int segmentIndex = (direction == Side::LEFT) ? adjTrap->leftSegmentIndex : adjTrap->rightSegmentIndex;

					while (true)
					{
						node = node->parent;

						if (node == state.treeRootNode)
						{
							assert(false);
							return;
						}

						if (node->type == TrapezoidationTreeNode::Type::SEGMENT &&
							node->elementIndex == segmentIndex)
						{
							countCrossings(segmentIndex, segmentCrossCounter);
							break;
						}
					}

					node = (direction == Side::LEFT) ? node->left : node->right;
				}
				else
				{
					// Helper function to test whether a trapezoid has a diagonal.
					auto hasDiagonal = [&state](Trapezoid* trap) -> bool
					{
						// A diagonal can be drawn between upper and lower points when those points are not on the same segment.

						int lpi = trap->lowerPointIndex;
						int upi = trap->upperPointIndex;

						auto& lseg = state.segments[trap->leftSegmentIndex];
						auto& rseg = state.segments[trap->rightSegmentIndex];

						return
							(lseg.lowerPointIndex != lpi || lseg.upperPointIndex != upi) &&
							(rseg.lowerPointIndex != lpi || rseg.upperPointIndex != upi);
					};

					// We have reached a trapezoid which is outside the polygon. Set the trapezoid status according to the fill rule.
					switch (state.fillRule)
					{
					case FillRule::NON_ZERO:
						if (segmentCrossCounter != 0)
						{
							trap->inside = true;
							trap->hasDiagonal = hasDiagonal(trap);
						}
						break;

					case FillRule::ODD:
						if ((segmentCrossCounter & 1) == 1)
						{
							trap->inside = true;
							trap->hasDiagonal = hasDiagonal(trap);
						}
						break;
					}
					
					finished = true;
				}

				break;
			}
			}
		}
	}
}

static void BuildTrapezoidTree(TriangulationState& state)
{
	// Generate random sequence of line segments.
	std::vector<size_t> segmentIndices;
	segmentIndices.resize(state.pointCoords.size());
	std::iota(segmentIndices.begin(), segmentIndices.end(), 0);
	if (state.randomizeSegments)
		std::shuffle(segmentIndices.begin(), segmentIndices.end(), state.rndEng);

	// Add each segment to the tree.
	for (size_t segInd : segmentIndices)
	{
		if (state.dbgSteps == 0)
			return;
		AddSegment(state, segInd);
	}

	DetermineInsideTrapezoids(state);
}

static void Triangulate(TriangulationState& state, std::vector<int>& monChain, Side monChainSide)
{
	size_t ib = 1;
	size_t prevOffs = (monChainSide == Side::LEFT) ? 1 : -1;
	size_t nextOffs = (monChainSide == Side::LEFT) ? -1 : 1;

	while (monChain.size() > 3)
	{
		size_t ia = ib + prevOffs;
		size_t ic = ib + nextOffs;
		auto& ptA = state.pointCoords[monChain[ia]];
		auto& ptB = state.pointCoords[monChain[ib]];
		auto& ptC = state.pointCoords[monChain[ic]];

		if (math3d::cross(math3d::vec3f { ptC - ptB, 0.0f }, math3d::vec3f { ptA - ptB, 0.0f }).z > 0)
		{
			// Convex vertex B.
			
			if (state.triangleWinding == Winding::CW)
				std::swap(ia, ic);

			state.outIndices.push_back(monChain[ia]);
			state.outIndices.push_back(monChain[ib]);
			state.outIndices.push_back(monChain[ic]);

			monChain.erase(monChain.begin() + ib);
		}
		else
		{
			++ib;
		}

		if (ib == monChain.size() - 1)
			ib = 1;
	}

	if ((monChainSide == Side::LEFT && state.triangleWinding == Winding::CW) ||
		(monChainSide == Side::RIGHT && state.triangleWinding == Winding::CCW))
	{
		state.outIndices.push_back(monChain[0]);
		state.outIndices.push_back(monChain[1]);
		state.outIndices.push_back(monChain[2]);
	}
	else
	{
		state.outIndices.push_back(monChain[2]);
		state.outIndices.push_back(monChain[1]);
		state.outIndices.push_back(monChain[0]);
	}
}

static void TraverseTrapezoids(TriangulationState& state, Trapezoid* trap, Side monChainSide)
{
	// Search down until a the lowest trapezoid of the monotone polygon is found,
	// that is the one who's lower point is the same as the lower point of the single segment.
	int singleSegInd = (monChainSide == Side::LEFT) ? trap->rightSegmentIndex : trap->leftSegmentIndex;
	auto singleSeg = &state.segments[singleSegInd];

	while (trap->lowerPointIndex != singleSeg->lowerPointIndex)
	{
		if (trap->lower2 != nullptr && monChainSide == Side::LEFT)
			trap = trap->lower2;
		else
			trap = trap->lower1;
	}

	// Do nothing if we have already started the chain with this trapezoid on the same side.
	size_t vi = static_cast<size_t>(monChainSide);
	if (trap->visited[vi])
		return;

	// Follow the monotone chain upward and add it's vertices to the list.
	std::vector<int> monChainVerts;
	monChainVerts.push_back(trap->lowerPointIndex);
	bool done = false;

	while (!done)
	{
		done = (trap->upperPointIndex == singleSeg->upperPointIndex);
		monChainVerts.push_back(trap->upperPointIndex);
		
		if (monChainSide == Side::LEFT)
		{
			if (trap->upper2 != nullptr)
			{
				if (!trap->visited[vi])
				{
					trap->visited[vi] = true;
					TraverseTrapezoids(state, trap, Side::RIGHT);
				}
				trap = trap->upper2;
			}
			else
			{
				if (!trap->visited[vi])
				{
					trap->visited[vi] = true;
					if (trap->hasDiagonal)
						TraverseTrapezoids(state, trap, Side::RIGHT);
				}
				trap = trap->upper1;
			}
		}
		else
		{
			if (trap->upper2 != nullptr)
			{
				if (!trap->visited[vi])
				{
					trap->visited[vi] = true;
					TraverseTrapezoids(state, trap, Side::LEFT);
				}
			}
			else
			{
				if (!trap->visited[vi])
				{
					trap->visited[vi] = true;
					if (trap->hasDiagonal)
						TraverseTrapezoids(state, trap, Side::LEFT);
				}
			}

			trap = trap->upper1;
		}
	}

	assert(monChainVerts.size() > 2);
	state.monChains.push_back(monChainVerts);
	Triangulate(state, monChainVerts, monChainSide);
}

static void BuildMonotonePolysAndTriangulate(TriangulationState& state)
{
	while (true)
	{
		Trapezoid* startTrap = nullptr;
		for (auto trap : state.trapezoids)
		{
			if (trap->inside &&
				!trap->visited[0] && !trap->visited[1] &&
				trap->lower1 == nullptr && trap->lower2 == nullptr)
			{
				startTrap = trap;
				break;
			}
		}

		if (startTrap == nullptr)
			return;

		auto& lseg = state.segments[startTrap->leftSegmentIndex];
		Side side = (lseg.upperPointIndex == startTrap->upperPointIndex) ? Side::LEFT : Side::RIGHT;
		TraverseTrapezoids(state, startTrap, side);
	}
}

bool TriangulatePolygon_Seidel(TriangulationState& state)
{
	size_t numPoints = state.pointCoords.size();

	if (numPoints < 3 ||
		numPoints > std::numeric_limits<unsigned short>::max())
	{
		return false;
	}

	const size_t numDiagonals = numPoints - 3;
	const size_t numTriangles = numPoints - 2;
	const size_t maxNumTrapezoids = 3 * numPoints + 1;

	Trapezoid::nextNumber = 1;

	BuildTrapezoidTree(state);

	if (state.dbgSteps == 0)
		return true;

	BuildMonotonePolysAndTriangulate(state);

	return true;
}

} // namespace BaseLib::Geometry
