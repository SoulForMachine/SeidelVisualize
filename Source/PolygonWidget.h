#ifndef _POLYGON_WIDGET_H_
#define _POLYGON_WIDGET_H_

#include <vector>
#include <unordered_set>
#include <utility>
#include <functional>
#include <memory>
#include <optional>
#include <nanogui/widget.h>
#include <Math/vec2.h>
#include "SeidelTriangulator.h"

class PolygonWidget : public nanogui::Widget
{
public:
	enum class State
	{
		AddingPoints,
		StepByStepTrapezoidation,
	};

	enum class ViewResult
	{
		None,
		Triangles,
		MonotoneChains,
	};

	struct Callbacks
	{
		std::function<void()> triangulatorUpdated;
		std::function<void(bool, bool)> polygonAvailable;
	};

	PolygonWidget(nanogui::Widget* parent, const Callbacks& callbacks);

	virtual void draw(NVGcontext* ctx) override;
	virtual bool mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers) override;
	virtual bool mouse_button_event(const nanogui::Vector2i& p, int button, bool down, int modifiers) override;
	virtual bool mouse_motion_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers) override;
	virtual bool scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) override;
	virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	virtual void perform_layout(NVGcontext* ctx) override;

	State GetState() const { return _state; }

	void SetPolygonOutlines(const OutlineList& outlines);
	void SetPolygonOutlines(OutlineList&& outlines);
	const OutlineList& GetPolygonOutlines() const { return _polygonOutlines; }

	void ClearAllOutlines();

	bool GetFillPolygons() const { return _fillPolygons; }
	void SetFillPolygons(bool fill) { _fillPolygons = fill; }

	ViewResult GetViewResult() const { return _viewResult; }
	void SetViewResult(ViewResult viewResults) { _viewResult = viewResults; }

	bool GetViewAxes() const { return _viewAxes; }
	void SetViewAxes(bool view) { _viewAxes = view; }

	bool GetViewGrid() const { return _viewGrid; }
	void SetViewGrid(bool view) { _viewGrid = view; }

	bool GetViewInfoOverlay() const { return _viewInfoOverlay; }
	void SetViewInfoOverlay(bool view) { _viewInfoOverlay = view; }

	bool GetViewTrapezoids() const { return _viewTrapezoids; }
	void SetViewTrapezoids(bool view) { _viewTrapezoids = view; }

	bool GetViewPoints() const { return _viewPoints; }
	void SetViewPoints(bool view) { _viewPoints = view; }

	bool GetViewPointNumbers() const { return _viewPointNumbers; }
	void SetViewPointNumbers(bool view) { _viewPointNumbers = view; }

	SeidelTriangulator::FillRule GetFillRule() const { return _fillRule; }
	void SetFillRule(SeidelTriangulator::FillRule fillRule);

	SeidelTriangulator::Winding GetTriangleWinding() const { return _triangleWinding; }
	void SetTriangleWinding(SeidelTriangulator::Winding winding);

	bool GetRandomizeSegments() const { return _randomizeSegments; }
	void SetRandomizeSegments(bool randomize);

	const SeidelTriangulator* GetTriangulator() const { return _triangulator.get(); }

	int_t GetNumTrapezoidationSteps() const;
	int_t GetNumTriangulationSteps() const;
	const IndexList& GetTriangleIndices() const { return _triangleIndices; }

	void EnableStepByStepTrapezoidation(bool enable);
	void SetCurrentStep(int_t currentStep, int_t trapSteps);

	void SetTrapNodeHighlight(const SeidelTriangulator::TreeNode* trapNode);
	void SetTrapNodeSelection(const SeidelTriangulator::TreeNode* trapNode);
	void CenterTrapNode(const SeidelTriangulator::TreeNode* trapNode);

private:
	struct PointDrawData
	{
		math3d::vec2f ptPos;
		math3d::vec2f numberVec;
		math3d::vec2f ptSSPos;
		math3d::vec2f numberSSPos;
		std::string numberStr;
		index_t outlineIndex;
		bool closeOutline;
	};

	struct TrapDrawData
	{
		std::vector<math3d::vec2f> points;
		math3d::vec2f numberPos;
		std::vector<math3d::vec2f> ssPoints;
		math3d::vec2f numberSSPos;
		std::string numberStr;
		bool inside;
		bool hasUpperSeg;
	};

	struct HighlightDrawData
	{
		const SeidelTriangulator::TreeNode* trapNode = nullptr;
		const TrapDrawData* trapDrawData = nullptr;
		std::optional<std::pair<math3d::vec2f, math3d::vec2f>> segmentDrawData;
		std::optional<math3d::vec2f> pointDrawData = std::nullopt;
	};

	void ResetView();
	void FillPolygons(NVGcontext* ctx);
	void DrawOutlines(NVGcontext* ctx);
	void DrawOutlinesStepByStepMode(NVGcontext* ctx);
	void DrawTrapezoids(NVGcontext* ctx);
	void DrawTriangleDiagonals(NVGcontext* ctx);
	void DrawTriangleDiagonalsStepByStepMode(NVGcontext* ctx);
	void DrawMonotoneChains(NVGcontext* ctx);
	void DrawMonotoneChainsStepByStepMode(NVGcontext* ctx);
	void DrawPointCursor(NVGcontext* ctx);
	void DrawBackground(NVGcontext* ctx);
	void DrawGrid(NVGcontext* ctx);
	void DrawAxes(NVGcontext* ctx);
	void DrawInfoOverlay(NVGcontext* ctx);
	void DrawDashedLine(NVGcontext* ctx, const math3d::vec2f& pt1, const math3d::vec2f& pt2);
	void DrawHighlightedTrapNode(NVGcontext* ctx, const HighlightDrawData& drawData, NVGcolor hlightColor);
	math3d::vec2f ScreenToWorld(const math3d::vec2f& screenPt);
	math3d::vec2f WorldToScreen(const math3d::vec2f& worldPt);
	void FinishCurrentOutline();
	bool IsNewSegmentValid(const math3d::vec2f& newPt, bool closing);
	void CalculateDrawingData();
	void CalculateOutlineData();
	void CalculateTrapezoidData();
	void UpdateScreenSpaceDrawingData();
	void UpdateHighlightDrawingData(HighlightDrawData& drawData);
	void RecreateTriangulator();
	void OnPolygonOutlinesChanged();
	void OnTrapezoidationParamChanged();
	void OnTriangulationParamChanged();
	void NotifyTriangulatorUpdated();
	void NotifyPolygonAvailable();
	void ClearTriangulationResults();
	math3d::vec2f SnapToAngle(const math3d::vec2f& pivotPt, const math3d::vec2f& snapPt, float angleDeg);
	math3d::vec2f SnapTo(const math3d::vec2f& snapPt, float value);
	math3d::vec2f AdjustedCursorPos(const nanogui::Vector2i& cursorPos);

	math3d::vec2f _pan = { 0.0f, 0.0f };
	float _zoom = 1.0f;
	OutlineList _polygonOutlines;
	Outline _curEditedOutline;
	State _state = State::AddingPoints;
	math3d::vec2f _cursorPos = { 0.0f, 0.0f };
	math3d::vec2f _cursorWorldPos = { 0.0f, 0.0f };
	bool _newSegmentValid = false;
	Callbacks _callbacks;
	std::unique_ptr<SeidelTriangulator> _triangulator = nullptr;
	SeidelTriangulator::TrapezoidationInfo _trapInfo = { };
	SeidelTriangulator::TriangulationInfo _triangInfo = { };
	IndexList _triangleIndices;
	IndexList _diagonalIndices;
	std::vector<IndexList> _monotoneChainsIndices;
	std::vector<nanogui::Widget*> _disabledInStepByStepMode;

	// Drawing data.
	math3d::vec2f _bboxMin = { 0.0f, 0.0f };
	math3d::vec2f _bboxMax = { 0.0f, 0.0f };
	std::vector<PointDrawData> _outlineDrawData;
	std::vector<TrapDrawData> _trapDrawData;
	HighlightDrawData _highlightDrawData;
	HighlightDrawData _selectionDrawData;

	// Options.
	bool _fillPolygons = true;
	ViewResult _viewResult = ViewResult::Triangles;
	bool _viewAxes = true;
	bool _viewGrid = true;
	bool _viewInfoOverlay = true;
	bool _viewTrapezoids = true;
	bool _viewPoints = true;
	bool _viewPointNumbers = true;
	SeidelTriangulator::FillRule _fillRule = SeidelTriangulator::FillRule::EvenOdd;
	SeidelTriangulator::Winding _triangleWinding = SeidelTriangulator::Winding::CCW;
	bool _randomizeSegments = true;
};

#endif // _POLYGON_WIDGET_H_
