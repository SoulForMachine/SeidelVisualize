#pragma once

#include <vector>
#include <QWidget>
#include <QPaintEvent>
#include "vec2.h"
#include "vec4.h"


namespace Geometry
{
struct TrapezoidationTreeNode;
struct TrapezoidTreeState;
}


class InputPolygonWidgetListener
{
public:
	virtual void OnEditFinished() = 0;
};


class InputPolygonWidget : public QWidget
{
	Q_OBJECT

public:
	InputPolygonWidget(QWidget *parent);
	~InputPolygonWidget();

	void SetListener(InputPolygonWidgetListener* listener);
	void SetTreeState(Geometry::TrapezoidTreeState* state);
	void ResetAndEdit();
	const std::vector<math3d::vec2f>& GetPoints() const;
	bool IsEditing() const { return _editing; }
	void SetPolygon(const std::vector<math3d::vec2f>& points);
	void ResetView();

protected:
	virtual void paintEvent(QPaintEvent* event) override;
	virtual void resizeEvent(QResizeEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual void keyPressEvent(QKeyEvent* event) override;

private:
	static constexpr float PIXELS_PER_METER = 100.0f;

	math3d::vec2f ScreenToWorld(const math3d::vec2i& screenPt);
	math3d::vec2i WorldToScreen(const math3d::vec2f& worldPt);
	void DrawTrapezoids(QPainter& painter, Geometry::TrapezoidationTreeNode* node);
	void RecalcBBox();

	Geometry::TrapezoidTreeState* _state = nullptr;
	InputPolygonWidgetListener* _listener = nullptr;
	math3d::vec2i _pan { 0, 0 };
	float _zoom { 1.0f };
	std::vector<math3d::vec2f> _points;
	math3d::vec2i _cursorPos { 0.0f, 0.0f };
	math3d::vec2i _prevPanPos { 0.0f, 0.0f };
	bool _editing = true;
	bool _panning = false;
	bool _initPan = true;
	QFont _font { "Verdana", 10 };
	math3d::vec4f _aabBox;
};
