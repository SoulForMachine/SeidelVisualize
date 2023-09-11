#ifndef _TRAP_TREE_WIDGET_H_
#define _TRAP_TREE_WIDGET_H_

#include <nanogui/widget.h>
#include <Math/vec2.h>
#include "SeidelTriangulator.h"
#include "Common.h"


class TrapTreeWidget : public nanogui::Widget
{
public:
	struct Callbacks
	{
		std::function<void(const SeidelTriangulator::TreeNode* trapNode)> hoveredNodeChanged;
		std::function<void(const SeidelTriangulator::TreeNode* trapNode)> selectedNodeChanged;
		std::function<void(const SeidelTriangulator::TreeNode* trapNode)> nodeDoubleClicked;
	};

	TrapTreeWidget(nanogui::Widget* parent, const Callbacks& callbacks);

	virtual void draw(NVGcontext* ctx) override;
	virtual bool mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers) override;
	virtual bool mouse_motion_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers) override;
	virtual bool mouse_button_event(const nanogui::Vector2i& p, int button, bool down, int modifiers) override;
	virtual bool mouse_enter_event(const nanogui::Vector2i& p, bool enter) override;
	virtual bool scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) override;
	virtual void perform_layout(NVGcontext* ctx) override;

	void SetTriangulator(const SeidelTriangulator* triangulator);
	void ResetView();

private:
	// This node structure is for building a parellel tree which
	// contains some additional data coresponding to the trapezoidation tree nodes.
	struct TreeDataNode
	{
		const SeidelTriangulator::TreeNode* trapNode = nullptr;
		int_t leftBreadth = 0;
		int_t rightBreadth = 0;

		math3d::vec2f position = { 0.0f, 0.0f };
		math3d::vec2f halfSize = { 0.0f, 0.0f };
		std::string caption;

		index_t leftIndex = -1;
		index_t rightIndex = -1;
	};

	void UpdateTreeData();
	index_t CreateTreeData(const SeidelTriangulator::TreeNode* trapNode);
	void UpdateTreeDrawingData(TreeDataNode& node, const math3d::vec2f& position, int level);
	void UpdateVisibility();
	const TreeDataNode* GetNodeAtPoint(const math3d::vec2f& screenPoint);
	void DrawTrapezoidationTree(NVGcontext* ctx);
	void NotifyHoveredNodeChanged();
	void NotifySelectedNodeChanged();
	void NotifyNodeDoubleClicked();

	const SeidelTriangulator* _triangulator = nullptr;
	std::vector<TreeDataNode> _treeDataNodes;
	std::vector<std::vector<const TreeDataNode*>> _visibilityMap;
	std::vector<const TreeDataNode*> _visibleNodes;
	std::vector<std::pair<math3d::vec2f, math3d::vec2f>> _visibleConnectingLines;
	math3d::vec2f _pan = { 0.0f, 0.0f };
	float _zoom = 1.0f;
	const TreeDataNode* _hoveredNode = nullptr;
	const TreeDataNode* _selectedNode = nullptr;
	Callbacks _callbacks;
};

#endif // _TRAP_TREE_WIDGET_H_
