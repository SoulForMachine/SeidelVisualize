#include "TrapTreeWidget.h"
#include <chrono>
#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanovg.h>
#include <GLFW/glfw3.h>
#include <Math/geometry.h>

constexpr NVGcolor BackgroundColor = { 0.7f, 0.7f, 0.7f, 1.0f };
constexpr NVGcolor NodeConnectorColor = { 0.0f, 0.0f, 0.0f, 1.0f };
constexpr NVGcolor NodeColor = { 1.0f, 1.0f, 1.0f, 1.0f };
constexpr NVGcolor NodeHighlightColor = { 0.4f, 0.8f, 0.4f, 1.0f };
constexpr NVGcolor NodeSelectedColor = { 0.8f, 0.4f, 0.4f, 1.0f };
constexpr NVGcolor TextColor = { 0.0f, 0.0f, 0.1f, 1.0f };
constexpr float MinZoom = 0.2f;
constexpr float MaxZoom = 5.0f;
constexpr float HorizontalNodeStep = 25.0f;
constexpr float VerticalNodeStep = 100.0f;
constexpr float MaxNodeHalfWidth = 30.0f;
constexpr float MaxNodeHalfHeight = 20.0f;
constexpr float PointNodeSquareHalfSize = MaxNodeHalfHeight;
constexpr math3d::vec2f SegmentNodeRectHalfSize = { MaxNodeHalfWidth, MaxNodeHalfHeight };
constexpr float TrapNodeCircleRadius = MaxNodeHalfHeight;

constexpr NVGcolor ArrowColor = { 0.8f, 0.2f, 0.0f, 1.0f };
constexpr math3d::vec2f ArrowVertices[] = {
	{ 0.0f, 0.0f },
	{ MaxNodeHalfHeight * 0.5f, -MaxNodeHalfHeight * 0.4f },
	{ MaxNodeHalfHeight * 0.15f, -MaxNodeHalfHeight * 0.4f },
	{ MaxNodeHalfHeight * 0.15f, -MaxNodeHalfHeight },
	{ -MaxNodeHalfHeight * 0.15f, -MaxNodeHalfHeight },
	{ -MaxNodeHalfHeight * 0.15f, -MaxNodeHalfHeight * 0.4f },
	{ -MaxNodeHalfHeight * 0.5f, -MaxNodeHalfHeight * 0.4f },
};

TrapTreeWidget::TrapTreeWidget(nanogui::Widget* parent, const Callbacks& callbacks) :
	nanogui::Widget(parent),
	_callbacks(callbacks)
{
	auto btnResetView = new nanogui::Button(this, "", FA_CROSSHAIRS);
	btnResetView->set_position({ 10, 10 });
	btnResetView->set_tooltip("Reset view");
	btnResetView->set_callback([this]() { ResetView(); });
}

void TrapTreeWidget::draw(NVGcontext* ctx)
{
	nvgSave(ctx);

	nvgTranslate(ctx, position().x(), position().y());

	// Fill background with the background color.
	nvgBeginPath(ctx);
	nvgRect(ctx, 0.0f, 0.0f, size().x(), size().y());
	nvgFillColor(ctx, BackgroundColor);
	nvgFill(ctx);

	// Draw the trapesoidation tree.
	if (_triangulator != nullptr)
	{
		auto w = static_cast<float>(width());
		auto h = static_cast<float>(height());
		nvgTranslate(ctx, w / 2.0f, h / 2.0f);
		nvgScale(ctx, _zoom, _zoom);
		nvgTranslate(ctx, _pan.x - w / 2.0f, _pan.y - h / 2.0f);

		DrawTrapezoidationTree(ctx);
	}
	
	nvgRestore(ctx);

	// Draw any child widgets.
	nanogui::Widget::draw(ctx);
}

bool TrapTreeWidget::mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers)
{
	if (_triangulator != nullptr && (button & (1 << GLFW_MOUSE_BUTTON_RIGHT)) != 0)
	{
		if ((modifiers & GLFW_MOD_SHIFT) != 0)
		{
			_zoom -= rel.y() * 0.005f;
			_zoom = std::clamp(_zoom, MinZoom, MaxZoom);
		}
		else
		{
			_pan += math3d::vec2f(rel.x(), rel.y()) / _zoom;
		}
		UpdateVisibility();
		return true;
	}

	return false;
}

bool TrapTreeWidget::mouse_motion_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers)
{
	if (nanogui::Widget::mouse_motion_event(p, rel, button, modifiers))
		return true;

	if (m_mouse_focus)
	{
		auto widgetCursorPos = p - m_pos + nanogui::Vector2i(1, 2);
		auto hoveredNode = GetNodeAtPoint(math3d::vec2f(widgetCursorPos.x(), widgetCursorPos.y()));
		if (hoveredNode != _hoveredNode)
		{
			_hoveredNode = hoveredNode;
			NotifyHoveredNodeChanged();
			return true;
		}
	}

	return false;
}

bool TrapTreeWidget::mouse_button_event(const nanogui::Vector2i& p, int button, bool down, int modifiers)
{
	if (nanogui::Widget::mouse_button_event(p, button, down, modifiers))
		return true;

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (down)
		{
			auto widgetCursorPos = p - m_pos;
			_selectedNode = GetNodeAtPoint(math3d::vec2f(widgetCursorPos.x(), widgetCursorPos.y()));
			NotifySelectedNodeChanged();
			return true;
		}
		else
		{
			// Double-click.
			static auto before = std::chrono::system_clock::now();
			auto now = std::chrono::system_clock::now();
			double diffMs = std::chrono::duration<double, std::milli>(now - before).count();
			before = now;
			if (diffMs > 10 && diffMs < 250)
			{
				if (_selectedNode != nullptr)
					NotifyNodeDoubleClicked();
				return true;
			}
		}
	}

	return false;
}

bool TrapTreeWidget::mouse_enter_event(const nanogui::Vector2i& p, bool enter)
{
	nanogui::Widget::mouse_enter_event(p, enter);

	if (!enter)
	{
		_hoveredNode = nullptr;
		NotifyHoveredNodeChanged();
		return true;
	}

	return false;
}

bool TrapTreeWidget::scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel)
{
	nanogui::Widget::scroll_event(p, rel);

	if (_triangulator != nullptr)
	{
		_zoom += rel.y() * 0.05f;
		_zoom = std::clamp(_zoom, MinZoom, MaxZoom);
		UpdateVisibility();
		return true;
	}

	return false;
}

void TrapTreeWidget::perform_layout(NVGcontext* ctx)
{
	UpdateVisibility();
	nanogui::Widget::perform_layout(ctx);
}

void TrapTreeWidget::SetTriangulator(const SeidelTriangulator* triangulator)
{
	_triangulator = triangulator;
	UpdateTreeData();
}

void TrapTreeWidget::ResetView()
{
	_pan.set_null();
	_zoom = 1.0f;
	UpdateVisibility();
}

void TrapTreeWidget::UpdateTreeData()
{
	_treeDataNodes.clear();
	_visibilityMap.clear();
	_visibleNodes.clear();
	_visibleConnectingLines.clear();

	_hoveredNode = nullptr;
	NotifyHoveredNodeChanged();

	_selectedNode = nullptr;
	NotifySelectedNodeChanged();

	if (_triangulator != nullptr)
	{
		auto rootNode = _triangulator->GetTreeRootNode();
		if (rootNode != nullptr)
		{
			index_t rootIndex = CreateTreeData(rootNode);
			UpdateTreeDrawingData(_treeDataNodes[rootIndex], { int(width() / 2.0f) + 0.5f, 100.5f }, 0);
			UpdateVisibility();
		}
	}
}

index_t TrapTreeWidget::CreateTreeData(const SeidelTriangulator::TreeNode* trapNode)
{
	_treeDataNodes.push_back(TreeDataNode());
	index_t index = _treeDataNodes.size() - 1;
	_treeDataNodes[index].trapNode = trapNode;

	if (trapNode->left != nullptr && trapNode->right != nullptr)
	{
		_treeDataNodes[index].leftIndex = CreateTreeData(trapNode->left);
		_treeDataNodes[index].rightIndex = CreateTreeData(trapNode->right);
		_treeDataNodes[index].leftBreadth = _treeDataNodes[_treeDataNodes[index].leftIndex].leftBreadth + _treeDataNodes[_treeDataNodes[index].leftIndex].rightBreadth + 1;
		_treeDataNodes[index].rightBreadth = _treeDataNodes[_treeDataNodes[index].rightIndex].leftBreadth + _treeDataNodes[_treeDataNodes[index].rightIndex].rightBreadth + 1;
	}
	else
	{
		_treeDataNodes[index].leftBreadth = 0;
		_treeDataNodes[index].rightBreadth = 0;
	}

	return index;
}

void TrapTreeWidget::UpdateTreeDrawingData(TreeDataNode& node, const math3d::vec2f& position, int level)
{
	const auto& segments = _triangulator->GetLineSegments();

	switch (node.trapNode->type)
	{
	case SeidelTriangulator::TreeNode::Type::Point:
	{
		node.position = position;
		node.halfSize.x = PointNodeSquareHalfSize;
		node.halfSize.y = PointNodeSquareHalfSize;
		node.caption = std::to_string(node.trapNode->elementIndex);
		break;
	}

	case SeidelTriangulator::TreeNode::Type::Segment:
	{
		node.position = position;
		node.halfSize = SegmentNodeRectHalfSize;
		node.caption =
			std::to_string(segments[node.trapNode->elementIndex].upperPointIndex) + ", " +
			std::to_string(segments[node.trapNode->elementIndex].lowerPointIndex);
		break;
	}

	case SeidelTriangulator::TreeNode::Type::Trapezoid:
	{
		node.position = position;
		node.halfSize.set(TrapNodeCircleRadius, TrapNodeCircleRadius);
		node.caption = std::to_string(node.trapNode->trapezoid->number);
		break;
	}
	}

	// Add the node to the visibility map. The first dimension represents node levels and the second one
	// contains nodes at a certain level sorted by x coordinate.

	if (level < _visibilityMap.size())
	{
		// If current level already exists, add this node to the list sorted by x coordinate.
		auto nodeXOrderPred = [](const auto& node1, const auto& node2) {
			return node1->position.x < node2->position.x;
		};
		auto insertIt = std::lower_bound(_visibilityMap[level].begin(), _visibilityMap[level].end(), &node, nodeXOrderPred);
		_visibilityMap[level].insert(insertIt, &node);
	}
	else
	{
		// Add this level to the map with the current node as the initial one.
		_visibilityMap.push_back({ &node });
		assert(_visibilityMap.size() == level + 1);
	}

	// Add children recursively.

	if (node.leftIndex >= 0)
	{
		math3d::vec2f leftPos;
		unsigned int steps = 1 + _treeDataNodes[node.leftIndex].rightBreadth;
		leftPos.x = position.x - HorizontalNodeStep * steps;
		leftPos.y = position.y + VerticalNodeStep;

		UpdateTreeDrawingData(_treeDataNodes[node.leftIndex], leftPos, level + 1);
	}

	if (node.rightIndex >= 0)
	{
		math3d::vec2f rightPos;
		unsigned int steps = 1 + _treeDataNodes[node.rightIndex].leftBreadth;
		rightPos.x = position.x + HorizontalNodeStep * steps;
		rightPos.y = position.y + VerticalNodeStep;

		UpdateTreeDrawingData(_treeDataNodes[node.rightIndex], rightPos, level + 1);
	}
}

void TrapTreeWidget::UpdateVisibility()
{
	_visibleNodes.clear();
	_visibleConnectingLines.clear();

	if (_treeDataNodes.empty())
		return;

	// Transform screen rectangle to nodes' coordinate system.

	auto w = static_cast<float>(width());
	auto h = static_cast<float>(height());

	math3d::vec2f scrPtMin(0.0f, 0.0f);
	scrPtMin -= math3d::vec2f(w / 2.0f, h / 2.0f);
	scrPtMin /= _zoom;
	scrPtMin += math3d::vec2f(-_pan.x + w / 2.0f, -_pan.y + h / 2.0f);

	math3d::vec2f scrPtMax(w, h);
	scrPtMax -= math3d::vec2f(w / 2.0f, h / 2.0f);
	scrPtMax /= _zoom;
	scrPtMax += math3d::vec2f(-_pan.x + w / 2.0f, -_pan.y + h / 2.0f);

	math3d::vec4f screenRect(scrPtMin, scrPtMax);

	// Determine which nodes are within the screen rectangle using the visibility map.

	index_t maxInd = _visibilityMap.size() - 1;
	float level0Y = _treeDataNodes[0].position.y - MaxNodeHalfHeight;
	index_t startLevel = std::clamp(static_cast<index_t>((scrPtMin.y - level0Y) / VerticalNodeStep), static_cast<index_t>(0), maxInd);
	index_t endLevel = std::clamp(static_cast<index_t>(std::floor((scrPtMax.y - level0Y) / VerticalNodeStep)), static_cast<index_t>(-1), maxInd);

	// Since level height is greater than node height, we need an additional check if the start level nodes
	// are above the upper viewport border.
	if (_visibilityMap[startLevel][0]->position.y + MaxNodeHalfHeight < scrPtMin.y)
		startLevel++;

	for (index_t level = startLevel; level <= endLevel; ++level)
	{
		const auto& nodesRow = _visibilityMap[level];
		// Find the first node which is to the right of the screen left border.
		auto startIt = std::lower_bound(
			nodesRow.begin(), nodesRow.end(), scrPtMin.x,
			[](const auto& node, float x) { return (node->position.x + node->halfSize.x) < x; });

		if (startIt != nodesRow.end())
		{
			// Find the last node which is to the left of the screen right border.
			auto endIt = std::upper_bound(
				startIt, nodesRow.end(), scrPtMax.x,
				[](float x, const auto& node) { return x < (node->position.x - node->halfSize.x); });

			for (auto it = startIt; it != endIt; ++it)
			{
				_visibleNodes.push_back(*it);
			}
		}
	}

	// Determine which node-connecting lines are within the screen rectangle. Include an aditional level before startLevel
	// if any, because the lines stemming from it might be visible even though the level itself isn't.

	for (index_t level = (startLevel > 0) ? startLevel - 1 : startLevel; level <= endLevel; ++level)
	{
		const auto& nodesRow = _visibilityMap[level];

		for (auto drawData : nodesRow)
		{
			if (drawData->leftIndex >= 0 && drawData->position.x > scrPtMin.x)
			{
				if (math3d::do_line_segment_rectangle_intersect_2d(
					drawData->position,
					_treeDataNodes[drawData->leftIndex].position,
					screenRect))
				{
					_visibleConnectingLines.push_back(std::make_pair(drawData->position, _treeDataNodes[drawData->leftIndex].position));
				}
			}

			if (drawData->rightIndex >= 0 && drawData->position.x < scrPtMax.x)
			{
				if (math3d::do_line_segment_rectangle_intersect_2d(
					drawData->position,
					_treeDataNodes[drawData->rightIndex].position,
					screenRect))
				{
					_visibleConnectingLines.push_back(std::make_pair(drawData->position, _treeDataNodes[drawData->rightIndex].position));
				}
			}
		}
	}
}

const TrapTreeWidget::TreeDataNode* TrapTreeWidget::GetNodeAtPoint(const math3d::vec2f& screenPoint)
{
	if (_treeDataNodes.empty())
		return nullptr;

	auto w = static_cast<float>(width());
	auto h = static_cast<float>(height());

	math3d::vec2f point = screenPoint;
	point -= math3d::vec2f(w / 2.0f, h / 2.0f);
	point /= _zoom;
	point += math3d::vec2f(-_pan.x + w / 2.0f, -_pan.y + h / 2.0f);

	float level0Y = _treeDataNodes[0].position.y - MaxNodeHalfHeight - 5.0f;
	index_t level = static_cast<index_t>(std::floor((point.y - level0Y) / VerticalNodeStep));

	if (level >= 0 && level < _visibilityMap.size())
	{
		const auto& nodesRow = _visibilityMap[level];
		// Find the first node who's center is to the right of the cursor position.
		auto it = std::lower_bound(
			nodesRow.begin(), nodesRow.end(), point.x,
			[](const auto& node, float x) { return node->position.x < x; });

		if (it == nodesRow.end())
		{
			// Check the last node.
			const TreeDataNode* node = *(it - 1);
			if (math3d::point_in_rectangle_2d(point, math3d::vec4f(node->position - node->halfSize, node->position + node->halfSize)))
				return node;
		}
		else
		{
			const TreeDataNode* node = *it;
			if (math3d::point_in_rectangle_2d(point, math3d::vec4f(node->position - node->halfSize, node->position + node->halfSize)))
				return node;

			if (it != nodesRow.begin())
			{
				// Check the node before.
				const TreeDataNode* node = *(it - 1);
				if (math3d::point_in_rectangle_2d(point, math3d::vec4f(node->position - node->halfSize, node->position + node->halfSize)))
					return node;
			}
		}
	}

	return nullptr;
}

void TrapTreeWidget::DrawTrapezoidationTree(NVGcontext* ctx)
{
	if (_treeDataNodes.empty())
		return;

	nvgSave(ctx);

	// Draw connecting lines.

	if (!_visibleConnectingLines.empty())
	{
		nvgBeginPath(ctx);

		for (const auto& line : _visibleConnectingLines)
		{
			nvgMoveTo(ctx, line.first.x, line.first.y);
			nvgLineTo(ctx, line.second.x, line.second.y);
		}

		nvgStrokeColor(ctx, NodeConnectorColor);
		nvgStroke(ctx);
	}

	// Draw root node arrow.

	const math3d::vec2f ArrowOffset(_treeDataNodes[0].position.x, _treeDataNodes[0].position.y - PointNodeSquareHalfSize * 1.3f);
	nvgTranslate(ctx, ArrowOffset.x, ArrowOffset.y);
	nvgBeginPath(ctx);

	nvgMoveTo(ctx, ArrowVertices[0].x, ArrowVertices[0].y);
	for (index_t i = 1; i < CountOf(ArrowVertices); ++i)
		nvgLineTo(ctx, ArrowVertices[i].x, ArrowVertices[i].y);
	nvgClosePath(ctx);

	nvgFillColor(ctx, ArrowColor);
	nvgFill(ctx);
	nvgTranslate(ctx, -ArrowOffset.x, -ArrowOffset.y);

	if (!_visibleNodes.empty())
	{
		auto drawNode = [ctx](const TreeDataNode& drawData) {
			switch (drawData.trapNode->type)
			{
			case SeidelTriangulator::TreeNode::Type::Point:
			{
				nvgRect(
					ctx,
					drawData.position.x - drawData.halfSize.x,
					drawData.position.y - drawData.halfSize.y,
					drawData.halfSize.x * 2.0f,
					drawData.halfSize.y * 2.0f);
				break;
			}

			case SeidelTriangulator::TreeNode::Type::Segment:
			{
				nvgRoundedRect(
					ctx,
					drawData.position.x - drawData.halfSize.x,
					drawData.position.y - drawData.halfSize.y,
					drawData.halfSize.x * 2.0f,
					drawData.halfSize.y * 2.0f,
					10.0f);
				break;
			}

			case SeidelTriangulator::TreeNode::Type::Trapezoid:
			{
				nvgCircle(ctx, drawData.position.x, drawData.position.y, drawData.halfSize.x);
				break;
			}
			}
		};

		// Draw filled node shapes.

		for (const auto drawData : _visibleNodes)
		{
			nvgBeginPath(ctx);

			drawNode(*drawData);

			NVGcolor color;
			if (_selectedNode != nullptr && drawData->trapNode == _selectedNode->trapNode)
				color = NodeSelectedColor;
			else if (_hoveredNode != nullptr && drawData->trapNode == _hoveredNode->trapNode)
				color = NodeHighlightColor;
			else
				color = NodeColor;

			nvgFillColor(ctx, color);
			nvgFill(ctx);
		}

		// Draw node shape outlines.

		nvgBeginPath(ctx);

		for (const auto drawData : _visibleNodes)
		{
			drawNode(*drawData);
		}

		nvgStrokeColor(ctx, NodeConnectorColor);
		nvgStroke(ctx);

		// Draw nodes' caption text.

		nvgFillColor(ctx, TextColor);
		nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFontSize(ctx, 20.0f);

		for (const auto drawData : _visibleNodes)
		{
			nvgText(ctx, drawData->position.x, drawData->position.y, drawData->caption.c_str(), nullptr);
		}
	}

	nvgRestore(ctx);
}

void TrapTreeWidget::NotifyHoveredNodeChanged()
{
	if (_callbacks.hoveredNodeChanged)
		_callbacks.hoveredNodeChanged((_hoveredNode != nullptr) ? _hoveredNode->trapNode : nullptr);
}

void TrapTreeWidget::NotifySelectedNodeChanged()
{
	if (_callbacks.selectedNodeChanged)
		_callbacks.selectedNodeChanged((_selectedNode != nullptr) ? _selectedNode->trapNode : nullptr);
}

void TrapTreeWidget::NotifyNodeDoubleClicked()
{
	if (_callbacks.nodeDoubleClicked)
		_callbacks.nodeDoubleClicked((_selectedNode != nullptr) ? _selectedNode->trapNode : nullptr);
}
