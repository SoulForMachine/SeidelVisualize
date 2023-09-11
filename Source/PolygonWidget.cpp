#include "PolygonWidget.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <nanogui/button.h>
#include <nanogui/checkbox.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/icons.h>
#include <nanogui/screen.h>
#include <nanovg.h>
#include <GLFW/glfw3.h>
#include <Math/mathutil.h>
#include <Math/geometry.h>
#include "Common.h"
#include "ComboWidget.h"
#include "PopupButtonWidget.h"

constexpr NVGcolor BackgroundColor = { 0.7f, 0.7f, 0.7f, 1.0f };
constexpr NVGcolor XAxisColor = { 0.5f, 0.25f, 0.25f, 1.0f };
constexpr NVGcolor YAxisColor = { 0.25f, 0.5f, 0.25f, 1.0f };
constexpr NVGcolor MajorGridColor = { 0.4f, 0.4f, 0.4f, 1.0f };
constexpr NVGcolor MinorGridColor = { 0.6f, 0.6f, 0.6f, 1.0f };
constexpr NVGcolor PolygonOutlineColor = { 0.1f, 0.1f, 0.5f, 1.0f };
constexpr NVGcolor PolygonOutlineErrorColor = { 0.6f, 0.1f, 0.1f, 1.0f };
constexpr NVGcolor PolygonOutlineGhostColor = { 0.1f, 0.1f, 0.5f, 0.2f };
constexpr NVGcolor PolygonVertexOuterColor = { 0.15f, 0.15f, 0.6f, 1.0f };
constexpr NVGcolor PolygonVertexInnerColor = { 0.5f, 0.5f, 0.9f, 1.0f };
constexpr NVGcolor PolygonVertexOuterGhostColor = { 0.15f, 0.15f, 0.6f, 0.2f };
constexpr NVGcolor PolygonVertexInnerGhostColor = { 0.5f, 0.5f, 0.9f, 0.2f };
constexpr NVGcolor VertexNumberColor = { 0.15f, 0.15f, 0.6f, 1.0f };
constexpr NVGcolor CurrentlyInsertedSegmentColor = { 0.7f, 0.43f, 0.0f, 1.0f };
constexpr NVGcolor CurrentlyInsertedVertexOuterColor = { 0.65f, 0.35f, 0.0f, 1.0f };
constexpr NVGcolor CurrentlyInsertedVertexInnerColor = { 0.87f, 0.61f, 0.0f, 1.0f };
constexpr NVGcolor VertexNumberGhostColor = { 0.15f, 0.15f, 0.6f, 0.2f };
constexpr NVGcolor InsideTrapNumberColor = { 0.1f, 0.5f, 0.1f, 1.0f };
constexpr NVGcolor OutsideTrapNumberColor = { 0.75f, 0.1f, 0.1f, 1.0f };
constexpr NVGcolor HorizLineColor = { 0.1f, 0.5f, 0.1f, 1.0f };
constexpr NVGcolor PolygonFillColor = { 0.0f, 0.2f, 0.6f, 0.1f };
constexpr NVGcolor DiagonalColor = { 0.9f, 0.0f, 0.0f, 1.0f };
constexpr NVGcolor CurrentlyInsertedMonChainSegColor = { 0.7f, 0.43f, 0.0f, 1.0f };
constexpr NVGcolor CurrentlyInsertedDiagonalColor = { 0.9f, 0.63f, 0.2f, 1.0f };
constexpr NVGcolor MonotoneChainColor = { 0.980f, 0.921f, 0.180f, 1.0f };
constexpr NVGcolor HighlightColorGreen = { 0.1f, 0.8f, 0.1f, 0.4f };
constexpr NVGcolor HighlightColorRed = { 0.8f, 0.1f, 0.1f, 0.4f };

constexpr float PixelsPerMeter = 100.0f;
constexpr float MinZoom = 0.2f;
constexpr float MaxZoom = 10.0f;
constexpr float GridStep = 0.5f;
constexpr float PolygonVertexOuterRadius = 5.5f;
constexpr float PolygonVertexInnerRadius = 3.5f;

PolygonWidget::PolygonWidget(nanogui::Widget* parent, const Callbacks& callbacks) :
	nanogui::Widget(parent),
	_callbacks(callbacks)
{
	// Create child widgets.

	// Button for reseting view transforms.
	{
		auto btnResetView = new nanogui::Button(this, "", FA_CROSSHAIRS);
		btnResetView->set_position({ 10, 10 });
		btnResetView->set_tooltip("Reset view");
		btnResetView->set_callback([this]() { ResetView(); });
	}

	// Button and popup panel with display options.
	{
		auto btnOptions = new PopupButtonWidget(this, "", FA_COG);
		btnOptions->set_position({ 50, 10 });
		btnOptions->set_tooltip("Display options");
		btnOptions->set_icon_extra_scale(1.0f);
		btnOptions->set_chevron_icon(0);
		// Change focus back to PolygonWidget when showing popup so it can get input events.
		btnOptions->set_change_callback([this](bool) { this->request_focus(); });

		auto popup = btnOptions->popup();
		popup->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 10, 10));

		auto viewCheckboxesWidget1 = new nanogui::Widget(popup);
		viewCheckboxesWidget1->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Minimum, 2, 10));
		auto chkAxes = new nanogui::CheckBox(viewCheckboxesWidget1, "Axes", [this](bool view) { SetViewAxes(view); });
		chkAxes->set_checked(GetViewAxes());
		auto chkGrid = new nanogui::CheckBox(viewCheckboxesWidget1, "Grid", [this](bool view) { SetViewGrid(view); });
		chkGrid->set_checked(GetViewGrid());
		auto chkInfoOverlay = new nanogui::CheckBox(viewCheckboxesWidget1, "Info overlay", [this](bool view) { SetViewInfoOverlay(view); });
		chkInfoOverlay->set_checked(GetViewInfoOverlay());

		auto viewCheckboxesWidget2 = new nanogui::Widget(popup);
		viewCheckboxesWidget2->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Minimum, 2, 10));
		auto chkFillPolys = new nanogui::CheckBox(viewCheckboxesWidget2, "Fill polygons", [this](bool fill) { SetFillPolygons(fill); });
		chkFillPolys->set_checked(GetFillPolygons());
		auto chkTraps = new nanogui::CheckBox(viewCheckboxesWidget2, "Trapezoids", [this](bool view) { SetViewTrapezoids(view); });
		chkTraps->set_checked(GetViewTrapezoids());
		auto chkPoints = new nanogui::CheckBox(viewCheckboxesWidget2, "Points", [this](bool view) { SetViewPoints(view); });
		chkPoints->set_checked(GetViewPoints());
		auto chkPointNumbers = new nanogui::CheckBox(viewCheckboxesWidget2, "Point numbers", [this](bool view) { SetViewPointNumbers(view); });
		chkPointNumbers->set_checked(GetViewPointNumbers());

		auto viewResultsWidget = new nanogui::Widget(popup);
		viewResultsWidget->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Fill));

		auto lblResults = new nanogui::Label(viewResultsWidget, "Results: ");
		auto cmbResults = new ComboWidget(viewResultsWidget, { "None", "Triangles", "Monotone chains" });
		cmbResults->set_callback([this](int result) { SetViewResult(static_cast<PolygonWidget::ViewResult>(result)); });
		cmbResults->set_selected_index(static_cast<int>(GetViewResult()));

		_disabledInStepByStepMode = {
			chkFillPolys,
			chkTraps,
			lblResults,
			cmbResults,
		};
	}
}

void PolygonWidget::draw(NVGcontext* ctx)
{
	DrawBackground(ctx);
	DrawGrid(ctx);
	DrawAxes(ctx);
	FillPolygons(ctx);
	DrawTrapezoids(ctx);

	if (_state != State::StepByStepTrapezoidation)
	{
		DrawTriangleDiagonals(ctx);
		DrawOutlines(ctx);
		DrawMonotoneChains(ctx);
	}
	else
	{
		DrawTriangleDiagonalsStepByStepMode(ctx);
		DrawOutlinesStepByStepMode(ctx);
		DrawMonotoneChainsStepByStepMode(ctx);
	}

	DrawHighlightedTrapNode(ctx, _selectionDrawData, HighlightColorRed);
	DrawHighlightedTrapNode(ctx, _highlightDrawData, HighlightColorGreen);
	DrawPointCursor(ctx);
	DrawInfoOverlay(ctx);

	nanogui::Widget::draw(ctx);
}

bool PolygonWidget::mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers)
{
	if ((button & (1 << GLFW_MOUSE_BUTTON_RIGHT)) != 0)
	{
		if ((modifiers & GLFW_MOD_SHIFT) != 0)
		{
			_zoom -= rel.y() * 0.005f;
			_zoom = std::clamp(_zoom, MinZoom, MaxZoom);
		}
		else
		{
			_pan += math3d::vec2f(rel.x(), -rel.y()) / (_zoom * PixelsPerMeter);
		}
		_cursorPos = AdjustedCursorPos(p);
		UpdateScreenSpaceDrawingData();
		return true;
	}

	return false;
}

bool PolygonWidget::mouse_button_event(const nanogui::Vector2i& p, int button, bool down, int modifiers)
{
	if (nanogui::Widget::mouse_button_event(p, button, down, modifiers))
		return true;

	if (down)
	{
		if (_state == State::AddingPoints)
		{
			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				auto widgetCursorPos = AdjustedCursorPos(p);
				math3d::vec2f worldPoint = ScreenToWorld(widgetCursorPos);
				bool ctrlPressed = ((modifiers & GLFW_MOD_CONTROL) != 0);
				bool shiftPressed = ((modifiers & GLFW_MOD_SHIFT) != 0);

				if (ctrlPressed && !shiftPressed)
				{
					if (!_curEditedOutline.empty())
					{
						const math3d::vec2f& prevPt = _curEditedOutline.back();
						worldPoint = SnapToAngle(prevPt, worldPoint, 45.0f);
					}
				}
				else if (shiftPressed && !ctrlPressed)
				{
					worldPoint = SnapTo(worldPoint, GridStep);
				}

				if (IsNewSegmentValid(worldPoint, false))
					_curEditedOutline.push_back(worldPoint);
			}
			else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
			{
				FinishCurrentOutline();
			}
		}
	}

	return true;
}

bool PolygonWidget::mouse_motion_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers)
{
	if (nanogui::Widget::mouse_motion_event(p, rel, button, modifiers))
		return true;

	if (_state != State::StepByStepTrapezoidation)
	{
		_cursorPos = AdjustedCursorPos(p);
		_cursorWorldPos = ScreenToWorld(_cursorPos);

		// Use GLFW becase modifiers parameter is set only when a button is pressed.
		auto glfwWnd = screen()->glfw_window();
		bool ctrlPressed = (glfwGetKey(glfwWnd, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);
		bool shiftPressed = (glfwGetKey(glfwWnd, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);

		if (ctrlPressed && !shiftPressed)
		{
			if (!_curEditedOutline.empty())
			{
				const math3d::vec2f& prevPt = _curEditedOutline.back();
				_cursorWorldPos = SnapToAngle(prevPt, _cursorWorldPos, 45.0f);
				_cursorPos = WorldToScreen(_cursorWorldPos);
			}
		}
		else if (shiftPressed && !ctrlPressed)
		{
			_cursorWorldPos = SnapTo(_cursorWorldPos, GridStep);
			_cursorPos = WorldToScreen(_cursorWorldPos);
		}

		// Set this flag for drawing.
		_newSegmentValid = IsNewSegmentValid(_cursorWorldPos, false);
		return true;
	}

	return false;
}

bool PolygonWidget::scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel)
{
	nanogui::Widget::scroll_event(p, rel);

	_zoom += rel.y() * 0.05f;
	_zoom = std::clamp(_zoom, MinZoom, MaxZoom);
	UpdateScreenSpaceDrawingData();
	return true;
}

bool PolygonWidget::keyboard_event(int key, int scancode, int action, int modifiers)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_SPACE:
			FinishCurrentOutline();
			return true;

		case GLFW_KEY_BACKSPACE:
			if (_state == State::AddingPoints && !_curEditedOutline.empty())
			{
				_curEditedOutline.pop_back();
				return true;
			}
			break;

		case GLFW_KEY_ESCAPE:
			if (_state == State::AddingPoints && !_curEditedOutline.empty())
			{
				_curEditedOutline.clear();
				return true;
			}
			break;
		}
	}

	return false;
}

void PolygonWidget::perform_layout(NVGcontext* ctx)
{
	UpdateScreenSpaceDrawingData();
	nanogui::Widget::perform_layout(ctx);
}

void PolygonWidget::SetPolygonOutlines(const OutlineList& outlines)
{
	_polygonOutlines = outlines;
	_pan.set_null();
	_zoom = 1.0f;
	OnPolygonOutlinesChanged();
}

void PolygonWidget::SetPolygonOutlines(OutlineList&& outlines)
{
	_polygonOutlines = std::move(outlines);
	_pan.set_null();
	_zoom = 1.0f;
	OnPolygonOutlinesChanged();
}

void PolygonWidget::ClearAllOutlines()
{
	_polygonOutlines.clear();
	_curEditedOutline.clear();
	OnPolygonOutlinesChanged();
}

void PolygonWidget::SetFillRule(SeidelTriangulator::FillRule fillRule)
{
	if (fillRule != _fillRule)
	{
		_fillRule = fillRule;
		OnTrapezoidationParamChanged();
	}
}

void PolygonWidget::SetTriangleWinding(SeidelTriangulator::Winding winding)
{
	if (winding != _triangleWinding)
	{
		_triangleWinding = winding;
		OnTriangulationParamChanged();
	}
}

void PolygonWidget::SetRandomizeSegments(bool randomize)
{
	if (randomize != _randomizeSegments)
	{
		_randomizeSegments = randomize;
		_trapInfo.segmentIndices.clear();	// Do not use old indices if any, generate new ones.
		OnTrapezoidationParamChanged();
	}
}

int_t PolygonWidget::GetNumTrapezoidationSteps() const
{
	return (_triangulator != nullptr && _triangulator->GetTreeRootNode() != nullptr) ?
		_trapInfo.numSteps : 0;
}

int_t PolygonWidget::GetNumTriangulationSteps() const
{
	return (_triangulator != nullptr) ? _triangInfo.numSteps : 0;
}

void PolygonWidget::EnableStepByStepTrapezoidation(bool enable)
{
	if (enable)
	{
		if (_state != State::StepByStepTrapezoidation &&
			_triangulator != nullptr &&
			_triangulator->GetTreeRootNode() != nullptr)
		{
			_state = State::StepByStepTrapezoidation;
			_triangulator->DeleteTrapezoidTree();
			_trapInfo.numSteps = 0;
			_trapInfo.segmentsAdded = 0;
			_trapInfo.upperPtIndex = -1;
			_trapInfo.lowerPtIndex = -1;
			_trapInfo.threadingSegmentIndex = -1;
			_trapInfo.thredingTrap = nullptr;
			NotifyTriangulatorUpdated();

			ClearTriangulationResults();
			_trapDrawData.clear();

			for (auto widget : _disabledInStepByStepMode)
				widget->set_enabled(false);
		}
	}
	else
	{
		if (_state == State::StepByStepTrapezoidation)
		{
			_state = State::AddingPoints;
			_trapInfo.maxSteps = -1;
			
			if (_triangulator->BuildTrapezoidTree(_trapInfo))
			{
				_triangInfo.maxSteps = -1;
				_triangulator->Triangulate(_triangInfo, _triangleIndices, _diagonalIndices, _monotoneChainsIndices);
			}
			else
			{
				ClearTriangulationResults();
			}

			NotifyTriangulatorUpdated();
			CalculateDrawingData();

			for (auto widget : _disabledInStepByStepMode)
				widget->set_enabled(true);
		}
	}
}

void PolygonWidget::SetCurrentStep(int_t currentStep, int_t trapSteps)
{
	if (_triangulator != nullptr)
	{
		if (currentStep <= trapSteps)
		{
			_trapInfo.maxSteps = currentStep;
			_triangulator->BuildTrapezoidTree(_trapInfo);
			CalculateTrapezoidData();
			ClearTriangulationResults();
		}
		else
		{
			if (_trapInfo.numSteps < trapSteps)
			{
				_trapInfo.maxSteps = trapSteps;
				_triangulator->BuildTrapezoidTree(_trapInfo);
				CalculateTrapezoidData();
			}

			_triangInfo.maxSteps = currentStep - trapSteps;
			_triangulator->Triangulate(_triangInfo, _triangleIndices, _diagonalIndices, _monotoneChainsIndices);
		}
		NotifyTriangulatorUpdated();

		UpdateScreenSpaceDrawingData();
	}
}

void PolygonWidget::SetTrapNodeHighlight(const SeidelTriangulator::TreeNode* trapNode)
{
	_highlightDrawData.trapNode = trapNode;
	UpdateHighlightDrawingData(_highlightDrawData);
}

void PolygonWidget::SetTrapNodeSelection(const SeidelTriangulator::TreeNode* trapNode)
{
	_selectionDrawData.trapNode = trapNode;
	UpdateHighlightDrawingData(_selectionDrawData);
}

void PolygonWidget::CenterTrapNode(const SeidelTriangulator::TreeNode* trapNode)
{
	// Pan the content so that the center point of the element is at the center of the widget.
	if (trapNode != nullptr && _triangulator != nullptr)
	{
		switch (trapNode->type)
		{
		case SeidelTriangulator::TreeNode::Type::Trapezoid:
		{
			// Center point of the trapezoid.
			const auto& trapezoids = _triangulator->GetTrapezoids();
			auto it = std::find(trapezoids.begin(), trapezoids.end(), trapNode->trapezoid);
			if (it != trapezoids.end())
			{
				const auto& trapDrawData = _trapDrawData[it - trapezoids.begin()];
				auto centerPt = math3d::centroid(trapDrawData.points.data(), trapDrawData.points.size());
				_pan = -centerPt;
				UpdateScreenSpaceDrawingData();
			}
			break;
		}
		case SeidelTriangulator::TreeNode::Type::Point:
		{
			const auto& points = _triangulator->GetPointCoords();
			_pan = -points[trapNode->elementIndex];
			UpdateScreenSpaceDrawingData();
			break;
		}
		case SeidelTriangulator::TreeNode::Type::Segment:
		{
			// Middle point of the segment.
			const auto& segments = _triangulator->GetLineSegments();
			const auto& points = _triangulator->GetPointCoords();
			const auto& pt1 = points[segments[trapNode->elementIndex].upperPointIndex];
			const auto& pt2 = points[segments[trapNode->elementIndex].lowerPointIndex];
			_pan = -(pt1 + pt2) / 2.0f;
			UpdateScreenSpaceDrawingData();
			break;
		}
		}
	}
}

void PolygonWidget::ResetView()
{
	_pan.set_null();
	_zoom = 1.0f;
	UpdateScreenSpaceDrawingData();
}

void PolygonWidget::FillPolygons(NVGcontext* ctx)
{
	if ((!_fillPolygons && _state != State::StepByStepTrapezoidation) || _trapDrawData.empty())
		return;

	nvgSave(ctx);

	nvgBeginPath(ctx);
	
	for (const auto& trap : _trapDrawData)
	{
		if (trap.inside)
		{
			nvgMoveTo(ctx, trap.ssPoints[0].x, trap.ssPoints[0].y);

			for (index_t i = 1; i < trap.ssPoints.size(); ++i)
				nvgLineTo(ctx, trap.ssPoints[i].x, trap.ssPoints[i].y);

			nvgClosePath(ctx);
		}
	}

	nvgFillColor(ctx, PolygonFillColor);
	nvgFill(ctx);

	nvgRestore(ctx);
}

void PolygonWidget::DrawOutlines(NVGcontext* ctx)
{
	nvgSave(ctx);

	nvgStrokeWidth(ctx, 3.0f);
	nvgLineJoin(ctx, NVG_ROUND);

	// Draw completed outlines.
	if (!_outlineDrawData.empty())
	{
		// Draw line segments.
		nvgBeginPath(ctx);

		bool firstPt = true;
		for (const auto& ptData : _outlineDrawData)
		{
			if (firstPt)
			{
				nvgMoveTo(ctx, ptData.ptSSPos.x, ptData.ptSSPos.y);
				firstPt = false;
			}
			else
			{
				nvgLineTo(ctx, ptData.ptSSPos.x, ptData.ptSSPos.y);
			}

			if (ptData.closeOutline)
			{
				nvgClosePath(ctx);
				firstPt = true;
			}
		}

		nvgStrokeColor(ctx, PolygonOutlineColor);
		nvgStroke(ctx);

		// Draw the vertices.
		if (_viewPoints)
		{
			nvgBeginPath(ctx);

			for (const auto& ptData : _outlineDrawData)
				nvgCircle(ctx, ptData.ptSSPos.x, ptData.ptSSPos.y, PolygonVertexOuterRadius);

			nvgFillColor(ctx, PolygonVertexOuterColor);
			nvgFill(ctx);

			nvgBeginPath(ctx);

			for (const auto& ptData : _outlineDrawData)
				nvgCircle(ctx, ptData.ptSSPos.x, ptData.ptSSPos.y, PolygonVertexInnerRadius);

			nvgFillColor(ctx, PolygonVertexInnerColor);
			nvgFill(ctx);
		}

		// Draw vertex numbers.
		if (_viewPointNumbers)
		{
			nvgFillColor(ctx, VertexNumberColor);
			nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			for (const auto& ptData : _outlineDrawData)
				nvgText(ctx, ptData.numberSSPos.x, ptData.numberSSPos.y, ptData.numberStr.c_str(), nullptr);
		}
	}

	// Draw the currently constructed outline.
	if (_state == State::AddingPoints)
	{
		if (!_curEditedOutline.empty())
		{
			// Calculate outline screen-space vertex positions.
			std::vector<math3d::vec2f> ssOutline(_curEditedOutline.size());

			for (index_t ptInd = 0; ptInd < _curEditedOutline.size(); ++ptInd)
			{
				const auto& pt = _curEditedOutline[ptInd];
				ssOutline[ptInd] = WorldToScreen({ pt.x, pt.y });
			}

			// Draw already inserted line segments.
			nvgBeginPath(ctx);

			auto ptIt = ssOutline.begin();
			nvgMoveTo(ctx, ptIt->x, ptIt->y);
			++ptIt;

			for (; ptIt != ssOutline.end(); ++ptIt)
				nvgLineTo(ctx, ptIt->x, ptIt->y);

			nvgStrokeColor(ctx, PolygonOutlineColor);
			nvgStroke(ctx);

			// Draw the currently edited line segment.
			nvgBeginPath(ctx);

			nvgMoveTo(ctx, ssOutline.back().x, ssOutline.back().y);
			nvgLineTo(ctx, _cursorPos.x, _cursorPos.y);

			nvgStrokeColor(ctx, _newSegmentValid ? PolygonOutlineColor : PolygonOutlineErrorColor);
			nvgStroke(ctx);

			// Draw polygon vertices.
			nvgBeginPath(ctx);

			for (const auto& pt : ssOutline)
				nvgCircle(ctx, pt.x, pt.y, PolygonVertexOuterRadius);

			nvgFillColor(ctx, PolygonOutlineColor);
			nvgFill(ctx);
		}
	}

	nvgRestore(ctx);
}

void PolygonWidget::DrawOutlinesStepByStepMode(NVGcontext* ctx)
{
	enum class InsertionState
	{
		NotInserted,
		Inserting,
		Inserted
	};

	const auto& lineSegments = _triangulator->GetLineSegments();

	std::vector<InsertionState> segmentStates(lineSegments.size(), InsertionState::NotInserted);
	std::vector<InsertionState> pointStates(_outlineDrawData.size(), InsertionState::NotInserted);

	for (int_t i = 0; i < _trapInfo.segmentsAdded; ++i)
	{
		auto segIndex = _trapInfo.segmentIndices[i];
		segmentStates[segIndex] = InsertionState::Inserted;
		pointStates[lineSegments[segIndex].upperPointIndex] = InsertionState::Inserted;
		pointStates[lineSegments[segIndex].lowerPointIndex] = InsertionState::Inserted;
	}

	math3d::vec2f curThreadingSegUpperPt = {};
	math3d::vec2f curThreadingSegLowerPt = {};

	if (_trapInfo.threadingSegmentIndex >= 0)
	{
		segmentStates[_trapInfo.threadingSegmentIndex] = InsertionState::Inserting;

		const auto& segment = lineSegments[_trapInfo.threadingSegmentIndex];
		curThreadingSegUpperPt = _outlineDrawData[segment.upperPointIndex].ptSSPos;
		curThreadingSegLowerPt = _outlineDrawData[segment.lowerPointIndex].ptSSPos;

		if (_trapInfo.thredingTrap != nullptr)
		{
			const auto& points = _triangulator->GetPointCoords();
			const auto& trapUpperPt = points[_trapInfo.thredingTrap->upperPointIndex];
			const auto& trapUpperLine = math3d::line_from_point_and_vec_2d(trapUpperPt, math3d::vec2f_x_axis);
			math3d::vec2f pt;
			if (math3d::intersect_lines_2d(pt, segment.line, trapUpperLine))
				curThreadingSegLowerPt = WorldToScreen(pt);
		}
	}

	if (_trapInfo.upperPtIndex >= 0)
		pointStates[_trapInfo.upperPtIndex] = InsertionState::Inserting;
	if (_trapInfo.lowerPtIndex >= 0)
		pointStates[_trapInfo.lowerPtIndex] = InsertionState::Inserting;

	nvgSave(ctx);

	nvgStrokeWidth(ctx, 3.0f);
	nvgLineJoin(ctx, NVG_ROUND);

	// Draw line segments.

	for (index_t segIndex = 0; segIndex < lineSegments.size(); ++segIndex)
	{
		auto upperPt = _outlineDrawData[lineSegments[segIndex].upperPointIndex].ptSSPos;
		auto lowerPt = _outlineDrawData[lineSegments[segIndex].lowerPointIndex].ptSSPos;

		NVGcolor color;
		switch (segmentStates[segIndex])
		{
		default:
		case InsertionState::NotInserted:
		case InsertionState::Inserting:
			color = PolygonOutlineGhostColor;
			break;
		case InsertionState::Inserted:
			color = PolygonOutlineColor;
			break;
		}

		nvgBeginPath(ctx);

		nvgMoveTo(ctx, upperPt.x, upperPt.y);
		nvgLineTo(ctx, lowerPt.x, lowerPt.y);

		nvgStrokeColor(ctx, color);
		nvgStroke(ctx);

		if (segmentStates[segIndex] == InsertionState::Inserting)
		{
			nvgBeginPath(ctx);

			nvgMoveTo(ctx, curThreadingSegUpperPt.x, curThreadingSegUpperPt.y);
			nvgLineTo(ctx, curThreadingSegLowerPt.x, curThreadingSegLowerPt.y);

			nvgStrokeColor(ctx, CurrentlyInsertedSegmentColor);
			nvgStroke(ctx);
		}
	}

	// Draw vertices.

	for (index_t ptIndex = 0; ptIndex < _outlineDrawData.size(); ++ptIndex)
	{
		// Skip inserted and not inserted points if viewing is off.
		if (!_viewPoints && pointStates[ptIndex] != InsertionState::Inserting)
			continue;

		NVGcolor innerColor;
		NVGcolor outerColor;
		switch (pointStates[ptIndex])
		{
		default:
		case InsertionState::NotInserted:
			innerColor = PolygonVertexInnerGhostColor;
			outerColor = PolygonVertexOuterGhostColor;
			break;
		case InsertionState::Inserting:
			innerColor = CurrentlyInsertedVertexInnerColor;
			outerColor = CurrentlyInsertedVertexOuterColor;
			break;
		case InsertionState::Inserted:
			innerColor = PolygonVertexInnerColor;
			outerColor = PolygonVertexOuterColor;
			break;
		}

		const auto& ptData = _outlineDrawData[ptIndex];

		nvgBeginPath(ctx);

		nvgCircle(ctx, ptData.ptSSPos.x, ptData.ptSSPos.y, PolygonVertexOuterRadius);

		nvgFillColor(ctx, outerColor);
		nvgFill(ctx);

		nvgBeginPath(ctx);

		nvgCircle(ctx, ptData.ptSSPos.x, ptData.ptSSPos.y, PolygonVertexInnerRadius);

		nvgFillColor(ctx, innerColor);
		nvgFill(ctx);
	}

	// Draw vertex numbers.
	nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	for (index_t ptIndex = 0; ptIndex < _outlineDrawData.size(); ++ptIndex)
	{
		// Skip inserted point numbers if viewing is off.
		if (!_viewPointNumbers && pointStates[ptIndex] != InsertionState::Inserting)
			continue;

		NVGcolor color;
		switch (pointStates[ptIndex])
		{
		default:
		case InsertionState::NotInserted:
			color = VertexNumberGhostColor;
			break;
		case InsertionState::Inserting:
		case InsertionState::Inserted:
			color = VertexNumberColor;
			break;
		}

		const auto& ptData = _outlineDrawData[ptIndex];

		nvgFillColor(ctx, color);
		nvgText(ctx, ptData.numberSSPos.x, ptData.numberSSPos.y, ptData.numberStr.c_str(), nullptr);
	}

	nvgRestore(ctx);
}

void PolygonWidget::DrawTrapezoids(NVGcontext* ctx)
{
	if ((!_viewTrapezoids && _state != State::StepByStepTrapezoidation) || _trapDrawData.empty())
		return;

	nvgSave(ctx);

	nvgBeginPath(ctx);

	for (const auto& trap : _trapDrawData)
	{
		if (trap.hasUpperSeg)
			DrawDashedLine(ctx, math3d::vec2f(trap.ssPoints[0]), math3d::vec2f(trap.ssPoints[1]));
	}

	nvgStrokeWidth(ctx, 1.0f);
	nvgLineJoin(ctx, NVG_ROUND);
	nvgStrokeColor(ctx, HorizLineColor);
	nvgStroke(ctx);

	nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

	for (const auto& trap : _trapDrawData)
	{
		if (trap.inside)
			nvgFillColor(ctx, InsideTrapNumberColor);
		else
			nvgFillColor(ctx, OutsideTrapNumberColor);

		nvgText(ctx, trap.numberSSPos.x, trap.numberSSPos.y, trap.numberStr.c_str(), nullptr);
	}

	nvgRestore(ctx);
}

void PolygonWidget::DrawTriangleDiagonals(NVGcontext* ctx)
{
	if (_viewResult == ViewResult::Triangles && !_diagonalIndices.empty())
	{
		nvgSave(ctx);

		nvgBeginPath(ctx);

		for (index_t i = 0; i < _diagonalIndices.size(); i += 2)
		{
			index_t index1 = _diagonalIndices[i];
			index_t index2 = _diagonalIndices[i + 1];
			nvgMoveTo(ctx, _outlineDrawData[index1].ptSSPos.x, _outlineDrawData[index1].ptSSPos.y);
			nvgLineTo(ctx, _outlineDrawData[index2].ptSSPos.x, _outlineDrawData[index2].ptSSPos.y);
		}

		nvgStrokeWidth(ctx, 1.0f);
		nvgLineCap(ctx, NVG_SQUARE);
		nvgStrokeColor(ctx, DiagonalColor);
		nvgStroke(ctx);

		nvgRestore(ctx);
	}
}

void PolygonWidget::DrawTriangleDiagonalsStepByStepMode(NVGcontext* ctx)
{
	if (!_diagonalIndices.empty())
	{
		bool highlightLastDiag = (_triangInfo.state == SeidelTriangulator::TriangulationInfo::State::AddingTriangle);
		const size_t numIndices = _diagonalIndices.size() - (highlightLastDiag ? 2 : 0);

		nvgSave(ctx);

		nvgBeginPath(ctx);

		for (index_t i = 0; i < _diagonalIndices.size(); i += 2)
		{
			index_t index1 = _diagonalIndices[i];
			index_t index2 = _diagonalIndices[i + 1];
			nvgMoveTo(ctx, _outlineDrawData[index1].ptSSPos.x, _outlineDrawData[index1].ptSSPos.y);
			nvgLineTo(ctx, _outlineDrawData[index2].ptSSPos.x, _outlineDrawData[index2].ptSSPos.y);
		}

		nvgStrokeWidth(ctx, 1.0f);
		nvgLineCap(ctx, NVG_SQUARE);
		nvgStrokeColor(ctx, DiagonalColor);
		nvgStroke(ctx);

		if (highlightLastDiag)
		{
			nvgBeginPath(ctx);

			index_t index1 = _diagonalIndices[numIndices];
			index_t index2 = _diagonalIndices[numIndices + 1];
			nvgMoveTo(ctx, _outlineDrawData[index1].ptSSPos.x, _outlineDrawData[index1].ptSSPos.y);
			nvgLineTo(ctx, _outlineDrawData[index2].ptSSPos.x, _outlineDrawData[index2].ptSSPos.y);

			nvgStrokeColor(ctx, CurrentlyInsertedDiagonalColor);
			nvgStroke(ctx);
		}

		nvgRestore(ctx);
	}
}

void PolygonWidget::DrawMonotoneChains(NVGcontext* ctx)
{
	if (_viewResult == ViewResult::MonotoneChains && !_monotoneChainsIndices.empty())
	{
		nvgSave(ctx);

		nvgBeginPath(ctx);

		for (const auto& monChain : _monotoneChainsIndices)
		{
			nvgMoveTo(ctx, _outlineDrawData[monChain[0]].ptSSPos.x, _outlineDrawData[monChain[0]].ptSSPos.y);
			for (index_t i = 1; i < monChain.size(); ++i)
				nvgLineTo(ctx, _outlineDrawData[monChain[i]].ptSSPos.x, _outlineDrawData[monChain[i]].ptSSPos.y);
		}

		nvgStrokeWidth(ctx, 1.0f);
		nvgLineCap(ctx, NVG_SQUARE);
		nvgLineJoin(ctx, NVG_ROUND);
		nvgStrokeColor(ctx, MonotoneChainColor);
		nvgStroke(ctx);

		nvgRestore(ctx);
	}
}

void PolygonWidget::DrawMonotoneChainsStepByStepMode(NVGcontext* ctx)
{
	if (!_monotoneChainsIndices.empty() && _triangInfo.state != SeidelTriangulator::TriangulationInfo::State::FinishedAll)
	{
		const auto& monChain = _monotoneChainsIndices.back();
		bool highlightLastChainSeg = (_triangInfo.state == SeidelTriangulator::TriangulationInfo::State::AddingMonChainSegment);
		const size_t numIndices = monChain.size() - (highlightLastChainSeg ? 1 : 0);

		nvgSave(ctx);

		nvgBeginPath(ctx);

		nvgMoveTo(ctx, _outlineDrawData[monChain[0]].ptSSPos.x, _outlineDrawData[monChain[0]].ptSSPos.y);
		for (index_t i = 1; i < numIndices; ++i)
			nvgLineTo(ctx, _outlineDrawData[monChain[i]].ptSSPos.x, _outlineDrawData[monChain[i]].ptSSPos.y);

		nvgStrokeWidth(ctx, 3.0f);
		nvgLineCap(ctx, NVG_ROUND);
		nvgLineJoin(ctx, NVG_ROUND);
		nvgStrokeColor(ctx, MonotoneChainColor);
		nvgStroke(ctx);

		if (highlightLastChainSeg)
		{
			nvgBeginPath(ctx);

			nvgMoveTo(ctx, _outlineDrawData[monChain[numIndices - 1]].ptSSPos.x, _outlineDrawData[monChain[numIndices - 1]].ptSSPos.y);
			nvgLineTo(ctx, _outlineDrawData[monChain[numIndices]].ptSSPos.x, _outlineDrawData[monChain[numIndices]].ptSSPos.y);

			nvgStrokeColor(ctx, CurrentlyInsertedMonChainSegColor);
			nvgStroke(ctx);
		}

		nvgRestore(ctx);
	}
}

void PolygonWidget::DrawPointCursor(NVGcontext* ctx)
{
	// New vertex at cursor position.
	if (_state == State::AddingPoints && m_mouse_focus)
	{
		nvgSave(ctx);

		nvgBeginPath(ctx);

		nvgCircle(ctx, _cursorPos.x, _cursorPos.y, PolygonVertexOuterRadius);

		nvgFillColor(ctx, _newSegmentValid ? PolygonOutlineColor : PolygonOutlineErrorColor);
		nvgFill(ctx);

		nvgRestore(ctx);
	}
}

void PolygonWidget::DrawBackground(NVGcontext* ctx)
{
	auto x1 = position().x();
	auto y1 = position().y();
	auto w = width();
	auto h = height();

	nvgSave(ctx);

	// Fill the whole area of the widget with the background color.
	nvgBeginPath(ctx);
	nvgRect(ctx, x1, y1, w, h);
	nvgFillColor(ctx, BackgroundColor);
	nvgFill(ctx);

	nvgRestore(ctx);
}

void PolygonWidget::DrawGrid(NVGcontext* ctx)
{
	if (!_viewGrid)
		return;

	auto x1 = position().x();
	auto y1 = position().y();
	auto w = width();
	auto h = height();
	auto x2 = x1 + w;
	auto y2 = y1 + h;

	auto wTopLeft = ScreenToWorld(math3d::vec2f(x1, y1));
	auto wBottomRight = ScreenToWorld(math3d::vec2f(x2, y2));
	float left = wTopLeft.x;
	float right = wBottomRight.x;
	float bottom = wBottomRight.y;
	float top = wTopLeft.y;

	float x = math3d::next_greater_eq_multiple(left, GridStep);
	float y = math3d::next_greater_eq_multiple(bottom, GridStep);

	nvgSave(ctx);

	// Draw the grid.
	nvgBeginPath(ctx);

	while (y <= top)
	{
		auto pt1 = WorldToScreen({ left, y });
		auto pt2 = WorldToScreen({ right, y });
		nvgMoveTo(ctx, pt1.x, pt1.y);
		nvgLineTo(ctx, pt2.x, pt2.y);

		y += GridStep;
	}

	while (x <= right)
	{
		auto pt1 = WorldToScreen({ x, bottom });
		auto pt2 = WorldToScreen({ x, top });
		nvgMoveTo(ctx, pt1.x, pt1.y);
		nvgLineTo(ctx, pt2.x, pt2.y);

		x += GridStep;
	}

	nvgStrokeColor(ctx, MinorGridColor);
	nvgStrokeWidth(ctx, 1.0f);
	nvgLineCap(ctx, NVG_SQUARE);
	nvgStroke(ctx);

	nvgRestore(ctx);
}

void PolygonWidget::DrawAxes(NVGcontext* ctx)
{
	if (!_viewAxes)
		return;

	auto x1 = position().x();
	auto y1 = position().y();
	auto w = width();
	auto h = height();
	auto x2 = x1 + w;
	auto y2 = y1 + h;

	auto wTopLeft = ScreenToWorld(math3d::vec2f(x1, y1));
	auto wBottomRight = ScreenToWorld(math3d::vec2f(x2, y2));

	float left = wTopLeft.x;
	float right = wBottomRight.x;
	float bottom = wBottomRight.y;
	float top = wTopLeft.y;

	bool xVis = (top > 0.0f && bottom < 0.0f);
	bool yVis = (left < 0.0f && right > 0.0f);

	// Draw coordinate system axes.
	if (xVis || yVis)
	{
		nvgSave(ctx);

		nvgStrokeWidth(ctx, 1.0f);
		nvgLineCap(ctx, NVG_SQUARE);

		if (xVis)
		{
			nvgBeginPath(ctx);
			
			auto pt1 = WorldToScreen({ left, 0.0f });
			auto pt2 = WorldToScreen({ right, 0.0f });
			nvgMoveTo(ctx, pt1.x, pt1.y);
			nvgLineTo(ctx, pt2.x, pt2.y);

			nvgStrokeColor(ctx, XAxisColor);
			nvgStroke(ctx);
		}
		if (yVis)
		{
			nvgBeginPath(ctx);

			auto pt1 = WorldToScreen({ 0.0f, top });
			auto pt2 = WorldToScreen({ 0.0f, bottom });
			nvgMoveTo(ctx, pt1.x, pt1.y);
			nvgLineTo(ctx, pt2.x, pt2.y);

			nvgStrokeColor(ctx, YAxisColor);
			nvgStroke(ctx);
		}

		nvgRestore(ctx);
	}
}

void PolygonWidget::DrawInfoOverlay(NVGcontext* ctx)
{
	if (!_viewInfoOverlay)
		return;

	nvgSave(ctx);
	nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
	nvgFontSize(ctx, 20.0f);

	constexpr float TextMarginAndPadding = 15.5f;
	const float textXPos = width() - TextMarginAndPadding;
	const float textStartYPos = TextMarginAndPadding;
	float textYPos = textStartYPos;
	float ascender, descender, lineHeight;
	nvgTextMetrics(ctx, &ascender, &descender, &lineHeight);

	struct InfoLine
	{
		std::string text;
		NVGcolor color;
		float yOffset;
	};

	std::vector<InfoLine> infoLines;
	float minX = width();

	auto addInfoLine = [&, ctx](std::string&& text, NVGcolor color) {
		math3d::vec4f bounds;
		nvgTextBounds(ctx, textXPos, textYPos, text.c_str(), nullptr, bounds);
		infoLines.push_back({ std::move(text), color, textYPos });
		textYPos += lineHeight;
		minX = std::min(minX, bounds.x);
	};

	constexpr NVGcolor StatisticTextColor = { 0.9f, 0.9f, 0.9f, 1.0f };
	constexpr NVGcolor TipTextColor = { 1.0f, 0.81f, 0.46f, 1.0f };
	constexpr NVGcolor ErrorTextColor = { 0.9f, 0.2f, 0.2f, 1.0f };
	constexpr NVGcolor InfoBackgroundColor = { 0.4f, 0.4f, 0.6f, 0.5f };

	// Print tips depending on the current mode.
	if (_state == State::AddingPoints)
	{
		addInfoLine("Left click - add new point", TipTextColor);
		
		if (_curEditedOutline.size() > 2)
			addInfoLine("Middle click / space - close the outline", TipTextColor);

		if (!_curEditedOutline.empty())
		{
			addInfoLine("Backspace - undo last point", TipTextColor);
			addInfoLine("Escape - undo edited outline", TipTextColor);
		}
	}
	else if (_state == State::StepByStepTrapezoidation)
	{
		addInfoLine("Right arrow - step forward", TipTextColor);
		addInfoLine("Left arrow - step back", TipTextColor);
		addInfoLine("Home - go to first step", TipTextColor);
		addInfoLine("End - go to last step", TipTextColor);
	}

	// Print current zoom level.
	{
		std::ostringstream sstream;
		sstream << std::fixed << std::setprecision(2) << _zoom;
		addInfoLine("Zoom: " + sstream.str(), StatisticTextColor);
	}

	// Print polygon statistics.
	if (_triangulator != nullptr)
	{
		if (_triangulator->IsSimplePolygon())
		{
			addInfoLine("Outlines: " + std::to_string(_polygonOutlines.size()), StatisticTextColor);
			addInfoLine("Points: " + std::to_string(_triangulator->GetPointCoords().size()), StatisticTextColor);

			if (!_triangleIndices.empty())
				addInfoLine("Triangles: " + std::to_string(_triangleIndices.size() / 3), StatisticTextColor);

			if (!_monotoneChainsIndices.empty())
				addInfoLine("Monotone chains: " + std::to_string(_monotoneChainsIndices.size()), StatisticTextColor);
		}
		else
		{
			addInfoLine("Not a simple polygon", ErrorTextColor);
		}
	}

	// Draw the transparent background.
	constexpr float TextPadding = 10.5f;
	nvgBeginPath(ctx);
	nvgRoundedRect(
		ctx, minX - TextPadding, textStartYPos - TextPadding,
		textXPos - minX + TextPadding * 2.0f,
		textYPos - textStartYPos + TextPadding * 2.0f,
		4.0f);
	nvgFillColor(ctx, InfoBackgroundColor);
	nvgFill(ctx);

	// Draw lines of text.
	for (const auto& line : infoLines)
	{
		nvgFillColor(ctx, line.color);
		nvgText(ctx, textXPos, line.yOffset, line.text.c_str(), nullptr);
	}

	// Draw world-space coordinates of cursor position.
	if (m_mouse_focus && _state == State::AddingPoints)
	{
		std::ostringstream sstream;
		sstream << std::fixed << std::setprecision(2) << u8"Shift - snap to grid  |  ";
		if (!_curEditedOutline.empty())
			sstream << u8"Ctlr - snap to 45\u00B0 angle  |  ";
		sstream << u8"X: " << _cursorWorldPos.x << u8"  Y: " << _cursorWorldPos.y;
		std::string mouseCoordsStr = sstream.str();

		math3d::vec4f bounds;
		nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
		nvgTextBounds(ctx, textXPos, height() - TextMarginAndPadding, mouseCoordsStr.c_str(), nullptr, bounds);

		nvgBeginPath(ctx);
		nvgRoundedRect(
			ctx, bounds.x - TextPadding, bounds.y - TextPadding,
			textXPos - bounds.x + TextPadding * 2.0f,
			(bounds.w - bounds.y) + TextPadding * 2.0f,
			4.0f);
		nvgFillColor(ctx, InfoBackgroundColor);
		nvgFill(ctx);

		nvgFillColor(ctx, StatisticTextColor);
		nvgText(ctx, textXPos, height() - TextMarginAndPadding, mouseCoordsStr.c_str(), nullptr);
	}

	nvgRestore(ctx);
}

void PolygonWidget::DrawDashedLine(NVGcontext* ctx, const math3d::vec2f& pt1, const math3d::vec2f& pt2)
{
	float length = (pt2 - pt1).length();
	auto vec = (pt2 - pt1) / length;
	float t = 0.0f;
	constexpr float DashLength = 15.0f;
	constexpr float SpaceLength = 6.0f;

	while (t < length)
	{
		float te = t + DashLength;
		float tn = te + SpaceLength;
		if (tn >= length - 1)
			te = length;

		auto dp1 = pt1 + vec * t;
		nvgMoveTo(ctx, dp1.x, dp1.y);
		auto dp2 = pt1 + vec * te;
		nvgLineTo(ctx, dp2.x, dp2.y);
		t = tn;
	}
}

void PolygonWidget::DrawHighlightedTrapNode(NVGcontext* ctx, const HighlightDrawData& drawData, NVGcolor hlightColor)
{
	bool hlightTrap = (drawData.trapDrawData != nullptr);
	bool hlightSegment = drawData.segmentDrawData.has_value();
	bool hlightPoint = drawData.pointDrawData.has_value();

	if (!hlightTrap && !hlightSegment && !hlightPoint)
		return;

	nvgSave(ctx);

	if (hlightTrap)
	{
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, drawData.trapDrawData->ssPoints[0].x, drawData.trapDrawData->ssPoints[0].y);

		for (index_t i = 1; i < drawData.trapDrawData->ssPoints.size(); ++i)
			nvgLineTo(ctx, drawData.trapDrawData->ssPoints[i].x, drawData.trapDrawData->ssPoints[i].y);

		nvgFillColor(ctx, hlightColor);
		nvgFill(ctx);
	}
	else if (hlightSegment)
	{
		nvgBeginPath(ctx);

		nvgMoveTo(ctx, drawData.segmentDrawData->first.x, drawData.segmentDrawData->first.y);
		nvgLineTo(ctx, drawData.segmentDrawData->second.x, drawData.segmentDrawData->second.y);

		nvgStrokeWidth(ctx, 9.0f);
		nvgLineCap(ctx, NVG_ROUND);
		nvgStrokeColor(ctx, hlightColor);
		nvgStroke(ctx);
	}
	else if (hlightPoint)
	{
		nvgBeginPath(ctx);

		nvgCircle(ctx, drawData.pointDrawData->x, drawData.pointDrawData->y, PolygonVertexOuterRadius + 6.0f);

		nvgFillColor(ctx, hlightColor);
		nvgFill(ctx);
	}

	nvgRestore(ctx);
}

math3d::vec2f PolygonWidget::ScreenToWorld(const math3d::vec2f& screenPt)
{
	auto w = static_cast<float>(width());
	auto h = static_cast<float>(height());
	math3d::vec2f centerPt(w / 2.0f, h / 2.0f);

	// Flip the y coordinate (world-space y axis increases upwards).
	// Apply units and zoom scaling to the point, centered at widget center.
	// Apply panning offset.
	return (math3d::vec2f(screenPt.x, h - screenPt.y) - centerPt) / (PixelsPerMeter * _zoom) - _pan;
}

math3d::vec2f PolygonWidget::WorldToScreen(const math3d::vec2f& worldPt)
{
	auto w = static_cast<float>(width());
	auto h = static_cast<float>(height());
	math3d::vec2f centerPt(w / 2.0f, h / 2.0f);

	// Translate the world point by panning offset.
	// Apply units and zoom scaling to the point.
	// Apply offset of the origin to the middle of the widget.
	math3d::vec2f screenPt = (worldPt + _pan) * PixelsPerMeter * _zoom + centerPt;

	// Flip the y coordinate (screen-space y axis increases downwards).
	screenPt.y = h - screenPt.y;

	// Screen-space point is rounded to a nearest half integer since pixel center is located at half integer coordinates in NanoVG.
	return {
		math3d::nearest_half(screenPt.x),
		math3d::nearest_half(screenPt.y)
	};
}

void PolygonWidget::FinishCurrentOutline()
{
	if (_state == State::AddingPoints &&
		_curEditedOutline.size() >= 3 &&
		IsNewSegmentValid(_curEditedOutline.front(), true))
	{
		_polygonOutlines.push_back(std::move(_curEditedOutline));
		OnPolygonOutlinesChanged();
	}
}

bool PolygonWidget::IsNewSegmentValid(const math3d::vec2f& newPt, bool closing)
{
	// If this is the first point of the new ouline, check that it's not on one of the segments of existing outlines.
	if (_curEditedOutline.empty())
	{
		for (const auto& outline : _polygonOutlines)
		{
			int_t numPts = outline.size();
			for (index_t i = 0; i < numPts; ++i)
			{
				if (math3d::is_point_on_segment(newPt, outline[i], outline[(i + 1) % numPts]))
					return false;
			}
		}

		return true;
	}

	const math3d::vec2f& prevPt = _curEditedOutline.back();

	// We don't want to create a zero-length segment.
	if (newPt == prevPt)
		return false;

	// The new segment must not intersect previously created outlines.
	for (auto& outline : _polygonOutlines)
	{
		int_t numPts = outline.size();
		for (index_t i = 0; i < numPts; ++i)
		{
			if (math3d::do_line_segments_intersect_2d(prevPt, newPt, outline[i], outline[(i + 1) % numPts]))
				return false;
		}
	}

	// Check the currently created outline.
	if (_curEditedOutline.size() < 2)
		return true;

	if (closing)
	{
		// If we are closing the outline, prevPt must equal the last point inserted and newPt must equal first.
		// This last segment must not intersect any previous segment.
		if (newPt != _curEditedOutline.front())
			return false;
	}

	int_t numSeg = _curEditedOutline.size() - 1;

	for (index_t i = 0; i < numSeg; ++i)
	{
		if ((i == 0 && closing) || (i == numSeg - 1))
		{
			if (math3d::do_line_segments_intersect_exclude_endpoints_2d(_curEditedOutline[i], _curEditedOutline[i + 1], prevPt, newPt))
			{
				return false;
			}
		}
		else
		{
			if (math3d::do_line_segments_intersect_2d(_curEditedOutline[i], _curEditedOutline[i + 1], prevPt, newPt))
			{
				return false;
			}
		}
	}

	return true;
}

void PolygonWidget::CalculateDrawingData()
{
	CalculateOutlineData();
	CalculateTrapezoidData();
	UpdateScreenSpaceDrawingData();
}

void PolygonWidget::CalculateOutlineData()
{
	_outlineDrawData.clear();

	if (_polygonOutlines.empty() || _triangulator == nullptr)
	{
		_bboxMin = { 0.0f, 0.0f };
		_bboxMax = { 0.0f, 0.0f };
		return;
	}

	_bboxMin = _bboxMax = _polygonOutlines[0][0];

	int_t pointNum = 0;
	const auto& outlinesWinding = _triangulator->GetOutlinesWinding();

	for (index_t outlInd = 0; outlInd < _polygonOutlines.size(); ++outlInd)
	{
		int_t numPts = _polygonOutlines[outlInd].size();

		for (index_t ptInd = 0; ptInd < numPts; ++ptInd)
		{
			PointDrawData ptData;
			const auto& pt = _polygonOutlines[outlInd][ptInd];
			ptData.ptPos = pt;

			// Calculate the point number vector. This is the direction in which to offset the number from the point position.
			// Normally, the number is placed outside of the polygon outline, except when the outside angle at that point is
			// less than 60 degrees, in which case it is placed inside the outline.
			const auto& nextPt = _polygonOutlines[outlInd][(ptInd + 1) % numPts];
			const auto& prevPt = _polygonOutlines[outlInd][(ptInd - 1 >= 0) ? ptInd - 1 : numPts - 1];
			auto vec1 = math3d::normalize(prevPt - pt);
			auto vec2 = math3d::normalize(nextPt - pt);

			if (outlinesWinding[outlInd] == SeidelTriangulator::Winding::CW)
				std::swap(vec1, vec2);

			float angle = math3d::angle_between_vectors_2d(vec1, vec2);
			if (math3d::fcmp_eq(angle, math3d::PI, 1.0e-3f))
				ptData.numberVec = math3d::normalize(math3d::rotate_90_ccw_2d(vec1));
			else if (angle < math3d::PI)
				ptData.numberVec = ((angle < math3d::deg2rad(60.0f)) ? -1.0f : 1.0f) * math3d::normalize(vec1 + vec2);
			else
				ptData.numberVec = -math3d::normalize(vec1 + vec2);

			ptData.numberStr = std::to_string(pointNum++);

			ptData.outlineIndex = outlInd;
			ptData.closeOutline = (ptInd == numPts - 1);

			_outlineDrawData.push_back(ptData);

			// Bounding box.
			if (pt.x < _bboxMin.x)
				_bboxMin.x = pt.x;
			if (pt.x > _bboxMax.x)
				_bboxMax.x = pt.x;

			if (pt.y < _bboxMin.y)
				_bboxMin.y = pt.y;
			if (pt.y > _bboxMax.y)
				_bboxMax.y = pt.y;
		}
	}
}

void PolygonWidget::CalculateTrapezoidData()
{
	_trapDrawData.clear();

	if (_triangulator == nullptr)
		return;

	// Calculate trapezoid points and horizontal lines.
	const auto& trapezoids = _triangulator->GetTrapezoids();
	const auto& points = _triangulator->GetPointCoords();
	const auto& segments = _triangulator->GetLineSegments();
	const float HorizExtent = (_bboxMax.x - _bboxMin.x) * 0.1f;
	const float VertExtent = (_bboxMax.y - _bboxMin.y) * 0.1f;
	const math3d::vec2f BoundMin(_bboxMin.x - HorizExtent, _bboxMin.y - VertExtent);
	const math3d::vec2f BoundMax(_bboxMax.x + HorizExtent, _bboxMax.y + VertExtent);

	for (const auto& trap : trapezoids)
	{
		TrapDrawData trapData;
		trapData.hasUpperSeg = (trap->upperPointIndex >= 0);

		// Calculate upper trapezoid points.
		if (trap->leftSegmentIndex >= 0 && trap->rightSegmentIndex >= 0 &&
			segments[trap->leftSegmentIndex].upperPointIndex == segments[trap->rightSegmentIndex].upperPointIndex &&
			segments[trap->leftSegmentIndex].upperPointIndex == trap->upperPointIndex)
		{
			// One upper point.
			trapData.points.push_back(points[trap->upperPointIndex]);
		}
		else
		{
			// Two upper points obtained by intersecting the upper horizontal line and two side line segments.
			// When any of these are missing, extended bounds are used instead.

			const math3d::vec2f upperPt = trapData.hasUpperSeg ? points[trap->upperPointIndex] : math3d::vec2f(BoundMin.x, BoundMax.y);
			const math3d::vec3f horizLine = math3d::line_from_points_2d(upperPt, upperPt + math3d::vec2f_x_axis);
			math3d::vec2f leftPt(BoundMin.x, upperPt.y);
			math3d::vec2f rightPt(BoundMax.x, upperPt.y);

			if (trap->leftSegmentIndex >= 0)
				if (!math3d::intersect_lines_2d(leftPt, segments[trap->leftSegmentIndex].line, horizLine))
					leftPt = upperPt;

			trapData.points.push_back(leftPt);

			if (trap->rightSegmentIndex >= 0)
				if (!math3d::intersect_lines_2d(rightPt, segments[trap->rightSegmentIndex].line, horizLine))
					rightPt = upperPt;

			trapData.points.push_back(rightPt);
		}

		// Calculate lower trapezoid points.
		if (trap->leftSegmentIndex >= 0 && trap->rightSegmentIndex >= 0 &&
			segments[trap->leftSegmentIndex].lowerPointIndex == segments[trap->rightSegmentIndex].lowerPointIndex &&
			segments[trap->leftSegmentIndex].lowerPointIndex == trap->lowerPointIndex)
		{
			// One lower point.
			trapData.points.push_back(points[trap->lowerPointIndex]);
		}
		else
		{
			// Two lower points obtained by intersecting the lower horizontal line and two side line segments.
			// When any of these are missing, extended bounds are used instead. Add the right one first and then
			// the left to make the correct trapezoid outline point order.

			const math3d::vec2f lowerPt = (trap->lowerPointIndex >= 0) ? points[trap->lowerPointIndex] : math3d::vec2f(BoundMin.x, BoundMin.y);
			const math3d::vec3f horizLine = math3d::line_from_points_2d(lowerPt, lowerPt + math3d::vec2f_x_axis);
			math3d::vec2f leftPt(BoundMin.x, lowerPt.y);
			math3d::vec2f rightPt(BoundMax.x, lowerPt.y);

			if (trap->rightSegmentIndex >= 0)
				if (!math3d::intersect_lines_2d(rightPt, segments[trap->rightSegmentIndex].line, horizLine))
					rightPt = lowerPt;

			trapData.points.push_back(rightPt);

			if (trap->leftSegmentIndex >= 0)
				if (!math3d::intersect_lines_2d(leftPt, segments[trap->leftSegmentIndex].line, horizLine))
					leftPt = lowerPt;

			trapData.points.push_back(leftPt);
		}

		trapData.numberPos = math3d::centroid(trapData.points.data(), trapData.points.size());
		trapData.numberStr = std::to_string(trap->number);
		trapData.inside = trap->inside;

		_trapDrawData.push_back(trapData);
	}
}

void PolygonWidget::UpdateScreenSpaceDrawingData()
{
	for (auto& ptData : _outlineDrawData)
	{
		ptData.ptSSPos = WorldToScreen(ptData.ptPos);
		ptData.numberSSPos = WorldToScreen(ptData.ptPos + ptData.numberVec * (0.15f / _zoom));
	}

	for (auto& trapData : _trapDrawData)
	{
		trapData.ssPoints.resize(trapData.points.size());
		for (index_t i = 0; i < trapData.points.size(); ++i)
			trapData.ssPoints[i] = WorldToScreen(trapData.points[i]);

		trapData.numberSSPos = WorldToScreen(trapData.numberPos);
	}

	UpdateHighlightDrawingData(_highlightDrawData);
	UpdateHighlightDrawingData(_selectionDrawData);
}

void PolygonWidget::UpdateHighlightDrawingData(HighlightDrawData& drawData)
{
	drawData.trapDrawData = nullptr;
	drawData.segmentDrawData.reset();
	drawData.pointDrawData.reset();

	auto trapNode = drawData.trapNode;
	if (trapNode == nullptr)
		return;

	switch (trapNode->type)
	{
	case SeidelTriangulator::TreeNode::Type::Trapezoid:
	{
		const auto& trapezoids = _triangulator->GetTrapezoids();
		auto it = std::find(trapezoids.begin(), trapezoids.end(), trapNode->trapezoid);
		if (it != trapezoids.end())
		{
			drawData.trapDrawData = &_trapDrawData[it - trapezoids.begin()];
		}
		break;
	}

	case SeidelTriangulator::TreeNode::Type::Segment:
	{
		const auto& segments = _triangulator->GetLineSegments();
		const auto& points = _triangulator->GetPointCoords();
		auto pt1 = WorldToScreen(points[segments[trapNode->elementIndex].upperPointIndex]);
		auto pt2 = WorldToScreen(points[segments[trapNode->elementIndex].lowerPointIndex]);
		drawData.segmentDrawData = std::make_pair(pt1, pt2);
		break;
	}

	case SeidelTriangulator::TreeNode::Type::Point:
	{
		const auto& points = _triangulator->GetPointCoords();
		drawData.pointDrawData = WorldToScreen(points[trapNode->elementIndex]);
		break;
	}
	}
}

void PolygonWidget::RecreateTriangulator()
{
	if (!_polygonOutlines.empty())
	{
		_triangulator = std::make_unique<SeidelTriangulator>(_polygonOutlines);
		_trapInfo = { };
		_trapInfo.fillRule = _fillRule;
		_trapInfo.randomizeSegments = _randomizeSegments;

		if (_triangulator->BuildTrapezoidTree(_trapInfo))
		{
			_triangInfo = { };
			_triangInfo.winding = _triangleWinding;
			_triangulator->Triangulate(_triangInfo, _triangleIndices, _diagonalIndices, _monotoneChainsIndices);
		}
		else
		{
			ClearTriangulationResults();
		}
	}
	else
	{
		_triangulator.reset();
		ClearTriangulationResults();
	}

	NotifyTriangulatorUpdated();
}

void PolygonWidget::OnPolygonOutlinesChanged()
{
	RecreateTriangulator();
	NotifyPolygonAvailable();
	CalculateDrawingData();
}

void PolygonWidget::OnTrapezoidationParamChanged()
{
	if (_triangulator != nullptr)
	{
		_trapInfo.fillRule = _fillRule;
		_trapInfo.randomizeSegments = _randomizeSegments;

		if (_triangulator->BuildTrapezoidTree(_trapInfo))
		{
			_triangInfo.winding = _triangleWinding;
			_triangulator->Triangulate(_triangInfo, _triangleIndices, _diagonalIndices, _monotoneChainsIndices);
		}
		else
		{
			ClearTriangulationResults();
		}

		NotifyTriangulatorUpdated();

		CalculateDrawingData();
	}
}

void PolygonWidget::OnTriangulationParamChanged()
{
	if (_triangulator != nullptr)
	{
		_triangInfo.winding = _triangleWinding;
		_triangulator->Triangulate(_triangInfo, _triangleIndices, _diagonalIndices, _monotoneChainsIndices);

		NotifyTriangulatorUpdated();
	}
}

void PolygonWidget::NotifyTriangulatorUpdated()
{
	if (_callbacks.triangulatorUpdated != nullptr)
		_callbacks.triangulatorUpdated();
}

void PolygonWidget::NotifyPolygonAvailable()
{
	if (_callbacks.polygonAvailable != nullptr)
		_callbacks.polygonAvailable(!_polygonOutlines.empty(), _triangulator != nullptr && _triangulator->IsSimplePolygon());
}

void PolygonWidget::ClearTriangulationResults()
{
	_triangleIndices.clear();
	_diagonalIndices.clear();
	_monotoneChainsIndices.clear();
}

math3d::vec2f PolygonWidget::SnapToAngle(const math3d::vec2f& pivotPt, const math3d::vec2f& snapPt, float angleDeg)
{
	math3d::vec2f vec = snapPt - pivotPt;
	float angle = math3d::angle_between_vectors_2d(math3d::vec2f_x_axis, math3d::normalize(vec));
	float snappedAngle = math3d::nearest_multiple(angle, math3d::deg2rad(45.0f));
	math3d::vec3f line = math3d::line_from_point_and_vec_2d(pivotPt, math3d::unit_vector_from_angle(snappedAngle));
	return math3d::nearest_point_on_line_2d(snapPt, line);
}

math3d::vec2f PolygonWidget::SnapTo(const math3d::vec2f& snapPt, float value)
{
	return {
		math3d::nearest_multiple(snapPt.x, value),
		math3d::nearest_multiple(snapPt.y, value)
	};
}

math3d::vec2f PolygonWidget::AdjustedCursorPos(const nanogui::Vector2i& cursorPos)
{
	// Calculate cursor position relative to this widget.
	// For some reason, NanoGUI subtracts Vector2i(1, 2) from cursor position in
	// Screen::cursor_pos_callback_event; add it back.
	// Add a half to coordinates so they are at a pixel center for drawing.
	auto widgetCursorPos = cursorPos - m_pos + nanogui::Vector2i(1, 2);
	return math3d::vec2f(widgetCursorPos.x() + 0.5f, widgetCursorPos.y() + 0.5f);
}
