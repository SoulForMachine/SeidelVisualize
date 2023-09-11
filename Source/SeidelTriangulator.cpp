#include "SeidelTriangulator.h"
#include <cassert>
#include <algorithm>
#include <numeric>
#include <Math/geometry.h>


SeidelTriangulator::SeidelTriangulator(const OutlineList& outlines)
{
	Init(outlines);
}

SeidelTriangulator::~SeidelTriangulator()
{
	Deinit();
}

bool SeidelTriangulator::BuildTrapezoidTree(TrapezoidationInfo& info)
{
	info.numSteps = 0;
	info.segmentsAdded = 0;
	info.upperPtIndex = -1;
	info.lowerPtIndex = -1;
	info.threadingSegmentIndex = -1;
	info.thredingTrap = nullptr;

	if (!_isSimplePolygon)
		return false;

	if (_treeRootNode != nullptr)
		DeleteTrapezoidTree();

	if (info.maxSteps == 0)
		return true;

	// If the caller did not supply the list of segment indices, generate random sequence of line segments
	// if requested or just a sequentially increasing index list if not.
	if (info.segmentIndices.empty())
	{
		info.segmentIndices.resize(_pointCoords.size());
		std::iota(info.segmentIndices.begin(), info.segmentIndices.end(), 0);
		if (info.randomizeSegments)
			std::shuffle(info.segmentIndices.begin(), info.segmentIndices.end(), _rndEng);
	}

	// Add each segment to the tree.
	for (index_t segInd : info.segmentIndices)
	{
		AddSegment(info, segInd);

		if (info.numSteps == info.maxSteps)
			return true;
	}

	DetermineInsideTrapezoids(info.fillRule);

	info.numSteps++;
	info.upperPtIndex = -1;
	info.lowerPtIndex = -1;
	info.threadingSegmentIndex = -1;
	info.thredingTrap = nullptr;

	return true;
}

void SeidelTriangulator::DeleteTrapezoidTree()
{
	for (auto trap : _trapezoids)
		delete trap;
	_trapezoids.clear();

	for (auto node : _treeNodes)
		delete node;
	_treeNodes.clear();

	_treeRootNode = nullptr;
	_nextTrapNumber = 1;

	for (auto& pt : _points)
		pt.node = nullptr;
}

bool SeidelTriangulator::Triangulate(TriangulationInfo& info, IndexList& outTriangleIndices, IndexList& outDiagonalIndices, std::vector<IndexList>& outMonotoneChains)
{
	info.numSteps = 0;
	info.state = TriangulationInfo::State::Undefined;

	if (_treeRootNode == nullptr)
		return false;

	outTriangleIndices.clear();
	outDiagonalIndices.clear();
	outMonotoneChains.clear();

	index_t startIndex = 0;
	for (auto& trap : _trapezoids)
		trap->visited[0] = trap->visited[1] = false;

	while (true)
	{
		SeidelTriangulator::Trapezoid* startTrap = nullptr;
		for (index_t i = startIndex; i < _trapezoids.size(); ++i)
		{
			auto trap = _trapezoids[i];
			if (trap->inside &&
				!trap->visited[0] && !trap->visited[1] &&
				trap->lower1 == nullptr && trap->lower2 == nullptr)
			{
				startTrap = trap;
				startIndex = i + 1;
				break;
			}
		}

		if (startTrap == nullptr)
			break;

		auto& lseg = _segments[startTrap->leftSegmentIndex];
		Side side = (lseg.upperPointIndex == startTrap->upperPointIndex) ? Side::Left : Side::Right;
		TraverseTrapezoids(info, outTriangleIndices, outDiagonalIndices, outMonotoneChains, startTrap, side);

		if (info.numSteps == info.maxSteps)
			return true;
	}

	info.numSteps++;
	info.state = TriangulationInfo::State::FinishedAll;

	return true;
}

void SeidelTriangulator::Init(const OutlineList& outlines)
{
	// Copy all points to a single array and count the total number of points.
	int_t numPoints = 0;
	bool invalid = false;
	for (auto& outl : outlines)
	{
		int_t n = outl.size();
		// Each outline must have at least 3 vertices.
		if (n < 3)
		{
			invalid = true;
		}

		_pointCoords.insert(_pointCoords.end(), outl.begin(), outl.end());
		numPoints += n;
	}

	_points.resize(numPoints);
	_segments.resize(numPoints);

	index_t i = 0;
	for (auto& outl : outlines)
	{
		float windingSum = 0.0f;

		for (index_t j = 0; j < outl.size(); ++j)
		{
			index_t index = i + j;
			Segment& seg = _segments[index];
			index_t ptAIndex = j;
			index_t ptBIndex = (j + 1) % outl.size();

			// Calculate polygon winding sum.
			windingSum += (outl[ptBIndex].x - outl[ptAIndex].x) * (outl[ptBIndex].y + outl[ptAIndex].y);

			// Determine which point is lower and which is upper.
			if (PointsVerticalRelation(outl[ptAIndex], outl[ptBIndex]) == VerticalRelation::Below)
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

			// Is the lower point located to the left of the upper point?
			bool lowerLeft = (PointsHorizontalRelation(_pointCoords[seg.lowerPointIndex], _pointCoords[seg.upperPointIndex]) == HorizontalRelation::Left);

			if (lowerLeft)
			{
				seg.leftPointIndex = seg.lowerPointIndex;
				seg.rightPointIndex = seg.upperPointIndex;
			}
			else
			{
				seg.leftPointIndex = seg.upperPointIndex;
				seg.rightPointIndex = seg.lowerPointIndex;
			}

			seg.line = math3d::line_from_points_2d(_pointCoords[seg.lowerPointIndex], _pointCoords[seg.upperPointIndex]);

			_points[index].node = nullptr;

			// Set segment 1-base indices for the upper and lower points. If a point is segment's right point,
			// then mark it by setting the segment index as negative.
			index_t segIndex = index + 1;
			if (_points[seg.lowerPointIndex].seg1Index == 0)
				_points[seg.lowerPointIndex].seg1Index = lowerLeft ? segIndex : -segIndex;
			else
				_points[seg.lowerPointIndex].seg2Index = lowerLeft ? segIndex : -segIndex;

			if (_points[seg.upperPointIndex].seg1Index == 0)
				_points[seg.upperPointIndex].seg1Index = lowerLeft ? -segIndex : segIndex;
			else
				_points[seg.upperPointIndex].seg2Index = lowerLeft ? -segIndex : segIndex;
		}

		// Winding for this ouline is clockwise if the sum is greater than 0.
		_outlinesWinding.push_back((windingSum > 0.0f) ? Winding::CW : Winding::CCW);

		i += outl.size();
	}

	_isSimplePolygon = !invalid && CheckIfSimplePolygon();
}

void SeidelTriangulator::Deinit()
{
	DeleteTrapezoidTree();
	// Clear polygon data.
	_pointCoords.clear();
	_points.clear();
	_segments.clear();
	_outlinesWinding.clear();

	_isSimplePolygon = false;
}

bool SeidelTriangulator::CheckIfSimplePolygon()
{
	// Shamos-Hoey sweep line algorithm is used to detect whether any line segments intersect.

	// A predicate for sorting segment point events.
	auto sortSegPtEventsPred = [this](index_t segPt1, index_t segPt2) -> bool {
		const auto& seg1 = _segments[std::abs(segPt1) - 1];
		const auto& seg2 = _segments[std::abs(segPt2) - 1];
		index_t pt1Index = (segPt1 > 0) ? seg1.leftPointIndex : seg1.rightPointIndex;
		index_t pt2Index = (segPt2 > 0) ? seg2.leftPointIndex : seg2.rightPointIndex;
		
		// If it is one and the same point shared by the two segments, the one which is
		// on the right of a segment is taken to be smaller.
		if (pt1Index == pt2Index)
			return (segPt1 < 0 && segPt2 > 0);
		
		const auto& pt1 = _pointCoords[pt1Index];
		const auto& pt2 = _pointCoords[pt2Index];

		// The point which is lexicographicaly on the left is smaller.
		return PointsHorizontalRelation(pt1, pt2) == HorizontalRelation::Left;
	};

	// A predicate used in std::adjacent_find(), to find two points with identical coordinates in a sorted vector.
	// Since each point appears twice, as a left point of a segment and as a right point of another
	// segment, we need to treat each such pair as distinct.
	auto adjEqPred = [this](index_t segPt1, index_t segPt2) -> bool {
		const auto& seg1 = _segments[std::abs(segPt1) - 1];
		const auto& seg2 = _segments[std::abs(segPt2) - 1];
		index_t pt1Index = (segPt1 > 0) ? seg1.leftPointIndex : seg1.rightPointIndex;
		index_t pt2Index = (segPt2 > 0) ? seg2.leftPointIndex : seg2.rightPointIndex;

		// If both indices refer to the same point, it means it's the shared point of the two segments.
		// Only looking for two different points with the same coordinates.
		if (pt1Index == pt2Index)
			return false;

		return _pointCoords[pt1Index] == _pointCoords[pt2Index];
	};

	// A predicate used for finding a place to insert a segment into a sorted vector of segments.
	// Segments are sorted by y coordinate of a new segment's left point and an intersection of
	// a vertical line going through that point and another segment.
	auto segOrderPred = [this](const Segment* otherSeg, const Segment* newSeg) -> bool {
		// Find the intersection of the vertical sweep line and the other segment.
		// If there is no intersection, use other segment's left point.
		const auto& leftEventPt = _pointCoords[newSeg->leftPointIndex];
		auto vertSweepLine = math3d::line_from_point_and_vec_2d(leftEventPt, math3d::vec2f_y_axis);
		math3d::vec2f otherPt;
		if (!math3d::intersect_lines_2d(otherPt, vertSweepLine, otherSeg->line))
			otherPt = _pointCoords[otherSeg->leftPointIndex];

		if (otherPt == leftEventPt)
		{
			// If points to be compared are the same, use right points of the segments.
			// Return true if other segment's right point is below new segment's right point.
			return (PointsVerticalRelation(_pointCoords[otherSeg->rightPointIndex], _pointCoords[newSeg->rightPointIndex]) == VerticalRelation::Below);
		}
		else
		{
			// Return true if other segment's point is below new segment's left point.
			return (PointsVerticalRelation(otherPt, leftEventPt) == VerticalRelation::Below);
		}
	};

	std::vector<const Segment*> sortedSegments;
	std::vector<index_t> segPtEvents(_points.size() * 2);

	for (index_t i = 0; i < _points.size(); ++i)
	{
		segPtEvents[2 * i] = _points[i].seg1Index;
		segPtEvents[2 * i + 1] = _points[i].seg2Index;
	}

	std::sort(segPtEvents.begin(), segPtEvents.end(), sortSegPtEventsPred);

	// No two equal points are allowed.
	auto eqIt = std::adjacent_find(segPtEvents.begin(), segPtEvents.end(), adjEqPred);
	if (eqIt != segPtEvents.end())
		return false;

	for (index_t segIndex : segPtEvents)
	{
		const Segment& seg = _segments[std::abs(segIndex) - 1];

		if (segIndex > 0)
		{
			// The point is the left endpoint of segment (starts the segment).
			auto insertPos = std::lower_bound(sortedSegments.begin(), sortedSegments.end(), &seg, segOrderPred);
			const Segment* nextSeg = (insertPos != sortedSegments.end()) ? *insertPos : nullptr;
			const Segment* prevSeg = (insertPos != sortedSegments.begin()) ? *std::prev(insertPos) : nullptr;

			if (prevSeg != nullptr)
			{
				if (seg.lowerPointIndex == prevSeg->lowerPointIndex || seg.upperPointIndex == prevSeg->lowerPointIndex ||
					seg.lowerPointIndex == prevSeg->upperPointIndex || seg.upperPointIndex == prevSeg->upperPointIndex)
				{
					// For adjacent segments use intersection test that excludes endpoints.
					if (math3d::do_line_segments_intersect_exclude_endpoints_2d(
						_pointCoords[seg.lowerPointIndex], _pointCoords[seg.upperPointIndex],
						_pointCoords[prevSeg->lowerPointIndex], _pointCoords[prevSeg->upperPointIndex]))
					{
						return false;
					}
				}
				else
				{
					if (math3d::do_line_segments_intersect_2d(
						_pointCoords[seg.lowerPointIndex], _pointCoords[seg.upperPointIndex],
						_pointCoords[prevSeg->lowerPointIndex], _pointCoords[prevSeg->upperPointIndex]))
					{
						return false;
					}
				}
			}

			if (nextSeg != nullptr)
			{
				if (seg.lowerPointIndex == nextSeg->lowerPointIndex || seg.upperPointIndex == nextSeg->lowerPointIndex ||
					seg.lowerPointIndex == nextSeg->upperPointIndex || seg.upperPointIndex == nextSeg->upperPointIndex)
				{
					// For adjacent segments use intersection test that excludes endpoints.
					if (math3d::do_line_segments_intersect_exclude_endpoints_2d(
						_pointCoords[seg.lowerPointIndex], _pointCoords[seg.upperPointIndex],
						_pointCoords[nextSeg->lowerPointIndex], _pointCoords[nextSeg->upperPointIndex]))
					{
						return false;
					}
				}
				else
				{
					if (math3d::do_line_segments_intersect_2d(
						_pointCoords[seg.lowerPointIndex], _pointCoords[seg.upperPointIndex],
						_pointCoords[nextSeg->lowerPointIndex], _pointCoords[nextSeg->upperPointIndex]))
					{
						return false;
					}
				}
			}

			sortedSegments.insert(insertPos, &seg);
		}
		else
		{
			// The point is the right endpoint of segment (ends the segment).
			auto segPos = std::find(sortedSegments.begin(), sortedSegments.end(), &seg);

			if (segPos != sortedSegments.end())
			{
				const Segment* nextSeg = (std::next(segPos) != sortedSegments.end()) ? *std::next(segPos) : nullptr;
				const Segment* prevSeg = (segPos != sortedSegments.begin()) ? *std::prev(segPos) : nullptr;

				if (prevSeg != nullptr && nextSeg != nullptr)
				{
					if (prevSeg->lowerPointIndex == nextSeg->lowerPointIndex || prevSeg->upperPointIndex == nextSeg->lowerPointIndex ||
						prevSeg->lowerPointIndex == nextSeg->upperPointIndex || prevSeg->upperPointIndex == nextSeg->upperPointIndex)
					{
						// For adjacent segments use intersection test that excludes endpoints.
						if (math3d::do_line_segments_intersect_exclude_endpoints_2d(
							_pointCoords[prevSeg->lowerPointIndex], _pointCoords[prevSeg->upperPointIndex],
							_pointCoords[nextSeg->lowerPointIndex], _pointCoords[nextSeg->upperPointIndex]))
						{
							return false;
						}
					}
					else
					{
						if (math3d::do_line_segments_intersect_2d(
							_pointCoords[prevSeg->lowerPointIndex], _pointCoords[prevSeg->upperPointIndex],
							_pointCoords[nextSeg->lowerPointIndex], _pointCoords[nextSeg->upperPointIndex]))
						{
							return false;
						}
					}
				}

				sortedSegments.erase(segPos);
			}
			else
			{
				assert(false);
			}
		}
	}

	assert(sortedSegments.empty());

	return true;
}

SeidelTriangulator::Trapezoid* SeidelTriangulator::AllocateTrapezoid()
{
	auto newTrap = new Trapezoid(_nextTrapNumber++);
	_trapezoids.push_back(newTrap);
	return newTrap;
}

void SeidelTriangulator::DeallocateTrapezoid(Trapezoid* trapezoid)
{
	for (auto it = _trapezoids.begin(); it != _trapezoids.end(); ++it)
	{
		if (*it == trapezoid)
		{
			_trapezoids.erase(it);
			break;
		}
	}

	delete trapezoid;
}

SeidelTriangulator::TreeNode* SeidelTriangulator::AllocateTrapTreeNode()
{
	auto newNode = new TreeNode;
	_treeNodes.push_back(newNode);
	return newNode;
}

void SeidelTriangulator::DeallocateTrapTreeNode(TreeNode* node)
{
	for (auto it = _treeNodes.begin(); it != _treeNodes.end(); ++it)
	{
		if (*it == node)
		{
			_treeNodes.erase(it);
			break;
		}
	}

	delete node;
}

// Add the point to the tree and return a pointer to the lower trapezoid.
// If the point has already been inserted, return nullptr.
SeidelTriangulator::Trapezoid* SeidelTriangulator::AddPoint(index_t pointIndex)
{
	// If the tree is empty, place a new vertex node as the root with two child trapezoid nodes.
	if (_treeRootNode == nullptr)
	{
		auto rightChild = AllocateTrapTreeNode();
		auto leftChild = AllocateTrapTreeNode();

		_treeRootNode = AllocateTrapTreeNode();
		_treeRootNode->type = TreeNode::Type::Point;
		_treeRootNode->elementIndex = pointIndex;
		_treeRootNode->left = leftChild;
		_treeRootNode->right = rightChild;

		// Node for upper trapezoid.
		rightChild->type = TreeNode::Type::Trapezoid;
		rightChild->parent = _treeRootNode;
		rightChild->trapezoid = AllocateTrapezoid();
		rightChild->trapezoid->lowerPointIndex = static_cast<int>(pointIndex);
		rightChild->trapezoid->node = rightChild;

		// Node for lower trapezoid.
		leftChild->type = TreeNode::Type::Trapezoid;
		leftChild->parent = _treeRootNode;
		leftChild->trapezoid = AllocateTrapezoid();
		leftChild->trapezoid->upperPointIndex = static_cast<int>(pointIndex);
		leftChild->trapezoid->node = leftChild;

		leftChild->trapezoid->upper1 = rightChild->trapezoid;
		rightChild->trapezoid->lower1 = leftChild->trapezoid;

		_points[pointIndex].node = _treeRootNode;

		return leftChild->trapezoid;
	}

	TreeNode* node = _treeRootNode;

	while (node != nullptr)
	{
		switch (node->type)
		{
		case TreeNode::Type::Point:
		{
			// If this is the same vertex, return the existing node.
			if (pointIndex == node->elementIndex)
				return nullptr;

			auto rel = PointsVerticalRelation(_pointCoords[pointIndex], _pointCoords[node->elementIndex]);
			node = (rel == VerticalRelation::Below) ? node->left : node->right;
			break;
		}

		case TreeNode::Type::Segment:
		{
			auto side = WhichSegmentSide(_pointCoords[pointIndex], _segments[node->elementIndex]);
			node = (side == Side::Left) ? node->left : node->right;
			break;
		}

		case TreeNode::Type::Trapezoid:
		{
			// We change this trapezoid node into a vertex node.
			// We split this trapezoid in two by the horizontal line that goes through the vertex
			// and add new trapezoid nodes as children of the vertex node.

			auto upperTrapezoidNode = AllocateTrapTreeNode();
			auto lowerTrapezoidNode = AllocateTrapTreeNode();

			auto oldTrap = node->trapezoid;
			auto newTrap = AllocateTrapezoid();

			// The new trapezoid node will reference the lower part of the trapezoid split by the vertex
			// and become the left child of the new vertex node.
			lowerTrapezoidNode->type = TreeNode::Type::Trapezoid;
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
			upperTrapezoidNode->type = TreeNode::Type::Trapezoid;
			upperTrapezoidNode->parent = node;
			upperTrapezoidNode->trapezoid = oldTrap;	// Reuse the trapezoid we are splitting as an upper part.
			upperTrapezoidNode->trapezoid->lowerPointIndex = static_cast<int>(pointIndex);
			upperTrapezoidNode->trapezoid->lower1 = lowerTrapezoidNode->trapezoid;
			upperTrapezoidNode->trapezoid->lower2 = nullptr;
			upperTrapezoidNode->trapezoid->node = upperTrapezoidNode;

			node->type = TreeNode::Type::Point;
			node->elementIndex = pointIndex;
			node->left = lowerTrapezoidNode;
			node->right = upperTrapezoidNode;

			_points[pointIndex].node = node;

			return lowerTrapezoidNode->trapezoid;
		}
		}
	}

	// Shouldn't reach this point. If it does, the tree has a leaf node which is not of type Trapezoid.
	return nullptr;
}

SeidelTriangulator::TreeNode* SeidelTriangulator::ThreadSegment(
	index_t segmentIndex,
	TreeNode* trapNode,
	TreeNode*& leftTrapNode,
	TreeNode*& rightTrapNode)
{
	TreeNode* nextTrapNode = nullptr;
	auto leftTrap = trapNode->trapezoid;	// Reuse the trapezoid we are splitting as a new left trapezoid.
	auto rightTrap = AllocateTrapezoid();
	auto& segment = _segments[segmentIndex];

	auto u1 = leftTrap->upper1;
	auto u2 = leftTrap->upper2;
	auto u3 = leftTrap->upper3;

	if (u1 != nullptr && u2 != nullptr)
	{
		// Two trapezoids above. It means the continuation of the thread.

		if (u3 != nullptr)
		{
			// There is a third upper neighbour.

			if (leftTrap->upper3Side == Trapezoid::ThirdUpperSide::Left)
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
				WhichSegmentSide(_pointCoords[segment.lowerPointIndex], _segments[ul1->rightSegmentIndex]) == Side::Right)
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
			auto side = WhichSegmentSide(_pointCoords[l1->upperPointIndex], segment);

			if (side == Side::Left)
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
					WhichSegmentSide(_pointCoords[segment.upperPointIndex], _segments[lu1->rightSegmentIndex]) == Side::Right)
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
					l1->upper3Side = Trapezoid::ThirdUpperSide::Right;
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
					l1->upper3Side = Trapezoid::ThirdUpperSide::Left;
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

	index_t rightSegIndex = leftTrap->rightSegmentIndex;

	// Create new left trapezoid node.
	leftTrapNode = AllocateTrapTreeNode();
	leftTrapNode->type = TreeNode::Type::Trapezoid;
	leftTrapNode->parent = trapNode;
	leftTrapNode->trapezoid = leftTrap;
	leftTrapNode->trapezoid->node = leftTrapNode;
	leftTrapNode->trapezoid->rightSegmentIndex = segmentIndex;

	// Create new right trapezoid node.
	rightTrapNode = AllocateTrapTreeNode();
	rightTrapNode->type = TreeNode::Type::Trapezoid;
	rightTrapNode->parent = trapNode;
	rightTrapNode->trapezoid = rightTrap;
	rightTrapNode->trapezoid->node = rightTrapNode;
	rightTrapNode->trapezoid->leftSegmentIndex = segmentIndex;
	rightTrapNode->trapezoid->rightSegmentIndex = rightSegIndex;
	rightTrapNode->trapezoid->upperPointIndex = leftTrap->upperPointIndex;
	rightTrapNode->trapezoid->lowerPointIndex = leftTrap->lowerPointIndex;

	// Change current trapezoid node into a segment node.
	trapNode->type = TreeNode::Type::Segment;
	trapNode->elementIndex = segmentIndex;
	trapNode->left = leftTrapNode;
	trapNode->right = rightTrapNode;

	return nextTrapNode;
}

SeidelTriangulator::TreeNode* SeidelTriangulator::GetFirstTrapezoidForNewSegment(TreeNode* startNode, const Segment& segment)
{
	TreeNode* node = startNode;

	while (node != nullptr)
	{
		switch (node->type)
		{
		case TreeNode::Type::Point:
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
					case TreeNode::Type::Point:
					{
						// Since this vertex is below upper vertex, the trapezoid must be above it.
						node = node->right;
						break;
					}

					case TreeNode::Type::Trapezoid:
					{
						return node;
					}

					case TreeNode::Type::Segment:
					{
						index_t ptIndex;

						// Procede to the side on which the other point is.
						if (_segments[node->elementIndex].lowerPointIndex == segment.upperPointIndex ||
							_segments[node->elementIndex].upperPointIndex == segment.upperPointIndex)
						{
							ptIndex = segment.lowerPointIndex;
						}
						else if (_segments[node->elementIndex].lowerPointIndex == segment.lowerPointIndex ||
							_segments[node->elementIndex].upperPointIndex == segment.lowerPointIndex)
						{
							ptIndex = segment.upperPointIndex;
						}
						else
						{
							//	assert(false);
							ptIndex = segment.upperPointIndex;
						}

						auto side = WhichSegmentSide(_pointCoords[ptIndex], _segments[node->elementIndex]);
						node = (side == Side::Left) ? node->left : node->right;
						break;
					}
					}
				}
			}
			else
			{
				auto rel = PointsVerticalRelation(_pointCoords[segment.upperPointIndex], _pointCoords[node->elementIndex]);
				node = (rel == VerticalRelation::Below) ? node->left : node->right;
			}

			break;
		}

		case TreeNode::Type::Segment:
		{
			auto side = WhichSegmentSide(_pointCoords[segment.upperPointIndex], _segments[node->elementIndex]);
			node = (side == Side::Left) ? node->left : node->right;
			break;
		}

		case TreeNode::Type::Trapezoid:
		{
			// The upper point is not inserted, this is the trapezoid where it should be.
			return node;
		}
		}
	}

	return nullptr;
}

SeidelTriangulator::TreeNode* SeidelTriangulator::MergeTrapezoids(TreeNode* prevTrapNode, TreeNode* curTrapNode)
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

		DeallocateTrapezoid(curTrap);
		DeallocateTrapTreeNode(curTrapNode);
		return prevTrapNode;
	}

	return curTrapNode;
}

void SeidelTriangulator::AddSegment(TrapezoidationInfo& trapInfo, index_t segmentIndex)
{
	Segment& segment = _segments[segmentIndex];
	Trapezoid* firstTrap = nullptr;

	// A predicate whose purpose is to limit the number of steps of the trapezoidation tree building
	// algorithm. It increments the counter each time it is called and returns false when maxSteps
	// is reached. If maxSteps is negative, there is no step limit.
	auto contPred = [&numSteps = trapInfo.numSteps, maxSteps = trapInfo.maxSteps ]() {
		if (maxSteps < 0)
		{
			++numSteps;
			return true;
		}
		else if (numSteps == maxSteps)
			return false;
		else
			return (++numSteps < maxSteps);
	};

	trapInfo.lowerPtIndex = -1;
	trapInfo.threadingSegmentIndex = -1;
	trapInfo.thredingTrap = nullptr;

	// First add upper and lower segment vertices to the tree.

	auto upperPtNode = _points[segment.upperPointIndex].node;
	if (upperPtNode == nullptr)
		firstTrap = AddPoint(segment.upperPointIndex);

	trapInfo.upperPtIndex = segment.upperPointIndex;

	if (!contPred())
		return;

	if (_points[segment.lowerPointIndex].node == nullptr)
		AddPoint(segment.lowerPointIndex);

	trapInfo.lowerPtIndex = segment.lowerPointIndex;

	if (!contPred())
		return;

	// Thread the segment from its upper point to its lower point through trapezoids and split
	// them in half.

	TreeNode* trapezoidNode = firstTrap ? firstTrap->node : GetFirstTrapezoidForNewSegment(upperPtNode, segment);
	TreeNode* prevLeftTrapNode = nullptr;
	TreeNode* prevRightTrapNode = nullptr;
	trapInfo.threadingSegmentIndex = segmentIndex;

	assert(trapezoidNode != nullptr);

	while (trapezoidNode->trapezoid->upperPointIndex != segment.lowerPointIndex)
	{
		TreeNode* leftTrapezoidNode;
		TreeNode* rightTrapezoidNode;
		TreeNode* nextTrapezoidNode;

		nextTrapezoidNode = ThreadSegment(segmentIndex, trapezoidNode, leftTrapezoidNode, rightTrapezoidNode);

		// After the split, merge left and right trapezoids with their upper neighbours if they are bounded by
		// the same left and right segments.
		prevLeftTrapNode = MergeTrapezoids(prevLeftTrapNode, leftTrapezoidNode);
		prevRightTrapNode = MergeTrapezoids(prevRightTrapNode, rightTrapezoidNode);

		trapezoidNode = nextTrapezoidNode;

		if (!contPred())
		{
			trapInfo.thredingTrap = trapezoidNode->trapezoid;
			break;
		}
	}

	if (trapezoidNode->trapezoid->upperPointIndex == segment.lowerPointIndex)
		++trapInfo.segmentsAdded;
}

void SeidelTriangulator::DetermineInsideTrapezoids(FillRule fillRule)
{
	auto countCrossings = [this](index_t segIndex, int_t& counter) {
		auto& segment = _segments[segIndex];

		if (segment.upward)
			counter--;
		else
			counter++;
	};

	for (auto trap : _trapezoids)
	{
		if (trap->lowerPointIndex < 0 ||
			trap->upperPointIndex < 0 ||
			trap->leftSegmentIndex < 0 ||
			trap->rightSegmentIndex < 0)
		{
			continue;
		}

		auto node = trap->node;
		Side direction = Side::Left;
		int_t segmentCrossCounter = 0;

		// Traverse upwards until the left or right segment of this trapezoid is encountered.
		// The segment side determines the direction.
		while (true)
		{
			node = node->parent;

			if (node == _treeRootNode)
			{
				assert(false);
				return;
			}

			if (node->type == TreeNode::Type::Segment)
			{
				if (node->elementIndex == trap->leftSegmentIndex)
				{
					countCrossings(trap->leftSegmentIndex, segmentCrossCounter);
					direction = Side::Left;
					break;
				}
				else if (node->elementIndex == trap->rightSegmentIndex)
				{
					countCrossings(trap->rightSegmentIndex, segmentCrossCounter);
					direction = Side::Right;
					break;
				}
			}
		}

		int_t pointCount = 0;
		bool finished = false;
		node = (direction == Side::Left) ? node->left : node->right;

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
			case TreeNode::Type::Point:
			{
				pointCount++;
				node = (pointCount % 2 == 1) ? node->left : node->right;
				break;
			}

			case TreeNode::Type::Segment:
			{
				node = (direction == Side::Left) ? node->right : node->left;
				break;
			}

			case TreeNode::Type::Trapezoid:
			{
				// We have reached an adjacent trapezoid.
				pointCount = 0;
				auto adjTrap = node->trapezoid;

				if (adjTrap->leftSegmentIndex >= 0 && adjTrap->rightSegmentIndex >= 0)
				{
					// This trapezoid has both left and right segments. Traverse upwards until the segment that matches
					// current direction is reached.
					index_t segmentIndex = (direction == Side::Left) ? adjTrap->leftSegmentIndex : adjTrap->rightSegmentIndex;

					while (true)
					{
						node = node->parent;

						if (node == _treeRootNode)
						{
							assert(false);
							return;
						}

						if (node->type == TreeNode::Type::Segment &&
							node->elementIndex == segmentIndex)
						{
							countCrossings(segmentIndex, segmentCrossCounter);
							break;
						}
					}

					node = (direction == Side::Left) ? node->left : node->right;
				}
				else
				{
					// Helper function to test whether a trapezoid has a diagonal.
					auto hasDiagonal = [this](Trapezoid* trap) -> bool
					{
						// A diagonal can be drawn between upper and lower points when those points are not on the same segment.

						index_t lpi = trap->lowerPointIndex;
						index_t upi = trap->upperPointIndex;

						auto& lseg = _segments[trap->leftSegmentIndex];
						auto& rseg = _segments[trap->rightSegmentIndex];

						return
							(lseg.lowerPointIndex != lpi || lseg.upperPointIndex != upi) &&
							(rseg.lowerPointIndex != lpi || rseg.upperPointIndex != upi);
					};

					// We have reached a trapezoid which is outside the polygon. Set the trapezoid status according to the fill rule.
					switch (fillRule)
					{
					case FillRule::NonZero:
						if (segmentCrossCounter != 0)
						{
							trap->inside = true;
							trap->hasDiagonal = hasDiagonal(trap);
						}
						break;

					case FillRule::EvenOdd:
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

void SeidelTriangulator::TraverseTrapezoids(TriangulationInfo& info, IndexList& outTriangleIndices, IndexList& outDiagonalIndices, std::vector<IndexList>& outMonotoneChains, Trapezoid* trap, Side monChainSide)
{
	// Search down until a the lowest trapezoid of the monotone polygon is found,
	// that is the one who's lower point is the same as the lower point of the single segment.
	index_t singleSegInd = (monChainSide == Side::Left) ? trap->rightSegmentIndex : trap->leftSegmentIndex;
	auto singleSeg = &_segments[singleSegInd];

	while (trap->lowerPointIndex != singleSeg->lowerPointIndex)
	{
		if (trap->lower2 != nullptr && monChainSide == Side::Left)
			trap = trap->lower2;
		else
			trap = trap->lower1;
	}

	// Do nothing if we have already started the chain with this trapezoid on the same side.
	index_t vi = static_cast<index_t>(monChainSide);
	if (trap->visited[vi])
		return;

	// Follow the monotone chain upward and add it's vertices to the list.
	IndexList monChainVerts;
	monChainVerts.push_back(trap->lowerPointIndex);
	bool done = false;
	Side otherSide = (monChainSide == Side::Left) ? Side::Right : Side::Left;
	index_t ovi = static_cast<index_t>(otherSide);
	std::vector<Trapezoid*> recurseTrapezoids;

	info.state = TriangulationInfo::State::AddingMonChainSegment;

	// A predicate whose purpose is to limit the number of steps of the triangulation algorithm.
	// It increments the counter each time it is called and returns false when maxSteps
	// is reached. If maxSteps is negative, there is no step limit.
	auto contPred = [&numSteps = info.numSteps, maxSteps = info.maxSteps]() {
		if (maxSteps < 0)
		{
			++numSteps;
			return true;
		}
		else if (numSteps == maxSteps)
			return false;
		else
			return (++numSteps < maxSteps);
	};

	while (!done)
	{
		done = (trap->upperPointIndex == singleSeg->upperPointIndex);
		monChainVerts.push_back(trap->upperPointIndex);

		if (!contPred())
		{
			outMonotoneChains.push_back(monChainVerts);
			return;
		}

		if (trap->upper2 != nullptr)
		{
			if (!trap->visited[vi])
			{
				trap->visited[vi] = true;
				if (trap->hasDiagonal && !trap->visited[ovi])
				{
					outDiagonalIndices.push_back(trap->upperPointIndex);
					outDiagonalIndices.push_back(trap->lowerPointIndex);
					recurseTrapezoids.push_back(trap);
				}
			}
			trap = (monChainSide == Side::Left) ? trap->upper2 : trap->upper1;
		}
		else
		{
			if (!trap->visited[vi])
			{
				trap->visited[vi] = true;
				if (trap->hasDiagonal && !trap->visited[ovi])
				{
					outDiagonalIndices.push_back(trap->upperPointIndex);
					outDiagonalIndices.push_back(trap->lowerPointIndex);
					recurseTrapezoids.push_back(trap);
				}
			}
			trap = trap->upper1;
		}
	}

	assert(monChainVerts.size() > 2);
	outMonotoneChains.push_back(monChainVerts);

	Triangulate(info, outTriangleIndices, outDiagonalIndices, monChainVerts, monChainSide);

	if (info.numSteps == info.maxSteps)
		return;

	for (auto trap : recurseTrapezoids)
	{
		TraverseTrapezoids(info, outTriangleIndices, outDiagonalIndices, outMonotoneChains, trap, otherSide);

		if (info.numSteps == info.maxSteps)
			return;
	}
}

void SeidelTriangulator::Triangulate(TriangulationInfo& info, IndexList& outTriangleIndices, IndexList& outDiagonalIndices, IndexList& monChain, Side monChainSide)
{
	index_t ib = 1;
	index_t prevOffs = (monChainSide == Side::Left) ? 1 : -1;
	index_t nextOffs = (monChainSide == Side::Left) ? -1 : 1;

	info.state = TriangulationInfo::State::AddingTriangle;

	// A predicate whose purpose is to limit the number of steps of the triangulation algorithm.
	// It increments the counter each time it is called and returns false when maxSteps
	// is reached. If maxSteps is negative, there is no step limit.
	auto contPred = [&numSteps = info.numSteps, maxSteps = info.maxSteps]() {
		if (maxSteps < 0)
		{
			++numSteps;
			return true;
		}
		else if (numSteps == maxSteps)
			return false;
		else
			return (++numSteps < maxSteps);
	};

	while (monChain.size() > 3)
	{
		index_t ia = ib + prevOffs;
		index_t ic = ib + nextOffs;
		const auto& ptA = _pointCoords[monChain[ia]];
		const auto& ptB = _pointCoords[monChain[ib]];
		const auto& ptC = _pointCoords[monChain[ic]];

		if (math3d::cross(math3d::vec3f(ptC - ptB, 0.0f), math3d::vec3f(ptA - ptB, 0.0f)).z > 0)
		{
			// Convex vertex B.

			if (info.winding == Winding::CW)
				std::swap(ia, ic);

			outTriangleIndices.push_back(monChain[ia]);
			outTriangleIndices.push_back(monChain[ib]);
			outTriangleIndices.push_back(monChain[ic]);

			outDiagonalIndices.push_back(monChain[ia]);
			outDiagonalIndices.push_back(monChain[ic]);

			monChain.erase(monChain.begin() + ib);

			if (!contPred())
				return;
		}
		else
		{
			++ib;
		}

		if (ib == monChain.size() - 1)
			ib = 1;
	}

	if ((monChainSide == Side::Left && info.winding == Winding::CW) ||
		(monChainSide == Side::Right && info.winding == Winding::CCW))
	{
		outTriangleIndices.push_back(monChain[0]);
		outTriangleIndices.push_back(monChain[1]);
		outTriangleIndices.push_back(monChain[2]);
	}
	else
	{
		outTriangleIndices.push_back(monChain[2]);
		outTriangleIndices.push_back(monChain[1]);
		outTriangleIndices.push_back(monChain[0]);
	}
}

SeidelTriangulator::VerticalRelation SeidelTriangulator::PointsVerticalRelation(const math3d::vec2f& queryPoint, const math3d::vec2f& inRelationToPoint)
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

SeidelTriangulator::HorizontalRelation SeidelTriangulator::PointsHorizontalRelation(const math3d::vec2f& queryPoint, const math3d::vec2f& inRelationToPoint)
{
	if (queryPoint.x < inRelationToPoint.x)
		return HorizontalRelation::Left;
	else if (queryPoint.x > inRelationToPoint.x)
		return HorizontalRelation::Right;
	else if (queryPoint.y < inRelationToPoint.y)
		return HorizontalRelation::Left;
	else
		return HorizontalRelation::Right;
}

SeidelTriangulator::Side SeidelTriangulator::WhichSegmentSide(const math3d::vec2f& point, const SeidelTriangulator::Segment& segment)
{
	if (math3d::point_to_line_sgn_dist_2d(point, segment.line) > 0.0f)
		return Side::Left;
	else
		return Side::Right;
}
