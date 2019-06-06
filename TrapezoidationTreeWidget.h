#pragma once

#include <QWidget>

namespace Geometry
{
struct TrapezoidationTreeNode;
struct TriangulationState;
}


class TrapezoidationTreeWidget : public QWidget
{
	Q_OBJECT

public:
	TrapezoidationTreeWidget(QWidget *parent);
	~TrapezoidationTreeWidget();
	
	void SetTreeState(Geometry::TriangulationState* state);
	void ResetView();

protected:
	virtual void paintEvent(QPaintEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void wheelEvent(QWheelEvent* event) override;

private:
	static constexpr int HORIZONTAL_NODE_STEP = 50;
	static constexpr int VERTICAL_NODE_STEP = 60;

	void UpdateNodeWeights(Geometry::TrapezoidationTreeNode* node);
	int GetTreeDepth(Geometry::TrapezoidationTreeNode* rootNode);
	void DrawNode(QPainter& painter, Geometry::TrapezoidationTreeNode* node, const QPoint& position, int level);

	Geometry::TriangulationState* _state = nullptr;
	QFont _font { "Verdana", 10 };
	bool _panning = false;
	QPoint _prevPanPos { 0, 0 };
	QPoint _pan { 0, 0 };
	float _zoom { 1.0f };
	int _numTreeLevels = 0;
};
