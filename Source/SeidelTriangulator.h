#ifndef _SEIDEL_TRIANGULATOR_H_
#define _SEIDEL_TRIANGULATOR_H_

#include <vector>
#include <random>
#include <Math/vec2.h>
#include <Math/vec3.h>
#include "Common.h"

using Outline = std::vector<math3d::vec2f>;
using OutlineList = std::vector<Outline>;
using IndexList = std::vector<index_t>;


class SeidelTriangulator
{
public:
	struct TreeNode;

	enum class FillRule
	{
		NonZero,
		EvenOdd,
	};

	enum class Winding
	{
		CW,
		CCW,
	};

	struct Point
	{
		// 1-based indices of segments that share this point.
		// If the point is left point of the segment, the index is positive.
		// If it is right point of the segment, the index is negative.
		index_t seg1Index = 0;
		index_t seg2Index = 0;

		TreeNode* node = nullptr;
	};

	struct Segment
	{
		index_t upperPointIndex;
		index_t lowerPointIndex;
		index_t leftPointIndex;
		index_t rightPointIndex;
		math3d::vec3f line;
		bool upward;
	};

	struct Trapezoid
	{
		Trapezoid(int_t num)
			: number(num)
		{ }

		enum class ThirdUpperSide
		{
			Left,
			Right
		};

		index_t upperPointIndex = -1;
		index_t lowerPointIndex = -1;
		Trapezoid* upper1 = nullptr;
		Trapezoid* upper2 = nullptr;
		Trapezoid* upper3 = nullptr;
		Trapezoid* lower1 = nullptr;
		Trapezoid* lower2 = nullptr;
		ThirdUpperSide upper3Side = ThirdUpperSide::Left;
		index_t leftSegmentIndex = -1;
		index_t rightSegmentIndex = -1;
		TreeNode* node = nullptr;
		bool inside = false;
		bool visited[2] = { false, false };
		bool hasDiagonal = false;

		const int_t number;
	};

	struct TreeNode
	{
		enum class Type
		{
			Point,
			Segment,
			Trapezoid
		};

		Type type;

		union
		{
			index_t elementIndex;	// When node type is Point or Segment.
			Trapezoid* trapezoid;	// When node type is Trapezoid.
		};

		TreeNode* left = nullptr;
		TreeNode* right = nullptr;
		TreeNode* parent = nullptr;
	};

	struct TrapezoidationInfo
	{
		// Input parameters. When passed in empty, segmentIndices are an output.
		FillRule fillRule = FillRule::EvenOdd;
		bool randomizeSegments = true;
		IndexList segmentIndices;
		int_t maxSteps = -1;

		// Output data.
		int_t numSteps = 0;
		int_t segmentsAdded = 0;
		index_t upperPtIndex = -1;
		index_t lowerPtIndex = -1;
		index_t threadingSegmentIndex = -1;
		Trapezoid* thredingTrap = nullptr;
	};

	struct TriangulationInfo
	{
		enum class State
		{
			Undefined,
			AddingMonChainSegment,
			AddingTriangle,
			FinishedAll
		};

		// Input parameters.
		Winding winding;
		int_t maxSteps = -1;

		// Output data.
		int_t numSteps = 0;
		State state = State::Undefined;
	};

	SeidelTriangulator(const OutlineList& outlines);
	~SeidelTriangulator();

	bool IsSimplePolygon() const { return _isSimplePolygon; }
	const TreeNode* GetTreeRootNode() const { return _treeRootNode; }
	const std::vector<Segment>& GetLineSegments() const { return _segments; }
	const std::vector<Trapezoid*>& GetTrapezoids() const { return _trapezoids; }
	const std::vector<math3d::vec2f>& GetPointCoords() const { return _pointCoords; }
	const std::vector<Winding>& GetOutlinesWinding() const { return _outlinesWinding; }

	bool BuildTrapezoidTree(TrapezoidationInfo& info);
	void DeleteTrapezoidTree();
	bool Triangulate(TriangulationInfo& info, IndexList& outTriangleIndices, IndexList& outDiagonalIndices, std::vector<IndexList>& outMonotoneChains);

private:
	enum class VerticalRelation
	{
		Above,
		Below
	};

	enum class HorizontalRelation
	{
		Left,
		Right
	};

	enum class Side
	{
		Left,
		Right
	};

	void Init(const OutlineList& outlines);
	void Deinit();
	bool CheckIfSimplePolygon();

	// Trapezoidation functions.
	Trapezoid* AllocateTrapezoid();
	void DeallocateTrapezoid(Trapezoid* trapezoid);
	TreeNode* AllocateTrapTreeNode();
	void DeallocateTrapTreeNode(TreeNode* node);
	Trapezoid* AddPoint(index_t pointIndex);
	TreeNode* ThreadSegment(index_t segmentIndex, TreeNode* trapNode, TreeNode*& leftTrapNode, TreeNode*& rightTrapNode);
	TreeNode* GetFirstTrapezoidForNewSegment(TreeNode* startNode, const Segment& segment);
	TreeNode* MergeTrapezoids(TreeNode* prevTrapNode, TreeNode* curTrapNode);
	void AddSegment(TrapezoidationInfo& trapInfo, index_t segmentIndex);
	void DetermineInsideTrapezoids(FillRule fillRule);

	// Triangulation functions.
	void TraverseTrapezoids(TriangulationInfo& info, IndexList& outTriangleIndices, IndexList& outDiagonalIndices, std::vector<IndexList>& outMonotoneChains, Trapezoid* trap, Side monChainSide);
	void Triangulate(TriangulationInfo& info, IndexList& outTriangleIndices, IndexList& outDiagonalIndices, IndexList& monChain, Side monChainSide);

	static VerticalRelation PointsVerticalRelation(const math3d::vec2f& queryPoint, const math3d::vec2f& inRelationToPoint);
	static HorizontalRelation PointsHorizontalRelation(const math3d::vec2f& queryPoint, const math3d::vec2f& inRelationToPoint);
	static Side WhichSegmentSide(const math3d::vec2f& point, const Segment& segment);

	std::vector<math3d::vec2f> _pointCoords;
	std::vector<Point> _points;
	std::vector<Segment> _segments;
	std::vector<TreeNode*> _treeNodes;
	std::vector<Trapezoid*> _trapezoids;
	std::vector<Winding> _outlinesWinding;
	TreeNode* _treeRootNode = nullptr;
	std::mt19937 _rndEng { std::random_device{}() };
	int_t _nextTrapNumber = 1;
	bool _isSimplePolygon = false;
};

#endif // _SEIDEL_TRIANGULATOR_H_
