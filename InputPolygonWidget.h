#pragma once

#include <vector>
#include <QWidget>
#include <QPaintEvent>
#include "vec2.h"
#include "vec4.h"


namespace Geometry
{
struct TrapezoidationTreeNode;
struct TriangulationState;
struct Trapezoid;
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
	enum class ResultViewType
	{
		Triangles,
		MonotoneChains,
		None
	};

	InputPolygonWidget(QWidget *parent);
	~InputPolygonWidget();

	void SetListener(InputPolygonWidgetListener* listener);
	void SetTreeState(Geometry::TriangulationState* state);
	void ResetAndEdit();
	bool IsEditing() const { return _editing; }
	const std::vector<std::vector<math3d::vec2f>>& GetOutlines() const { return _outlines; }
	void SetOutlines(const std::vector<std::vector<math3d::vec2f>>& outlines);
	void SetOutlines(std::vector<std::vector<math3d::vec2f>>&& outlines);
	void ResetView();

	bool GetViewTrapezoids() const { return _viewTraps; }
	void SetViewTrapezoids(bool view);
	ResultViewType GetResultViewType() const { return _resultViewType; }
	void SetResultViewType(ResultViewType type);

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
	void DrawAxes(QPainter& painter);
	void DrawOutlines(QPainter& painter);
	void DrawTrapezoids(QPainter& painter);
	void DrawTriangles(QPainter& painter);
	void DrawMonotoneChains(QPainter& painter);
	void RecalcBBox();
	bool IsNewSegmentValid(const math3d::vec2f& newPt, bool closing);

	Geometry::TriangulationState* _state = nullptr;
	InputPolygonWidgetListener* _listener = nullptr;
	math3d::vec2i _pan { 0, 0 };
	float _zoom { 1.0f };
	std::vector<math3d::vec2f> _points;
	std::vector<std::vector<math3d::vec2f>> _outlines;
	math3d::vec2i _cursorPos { 0.0f, 0.0f };
	math3d::vec2i _prevPanPos { 0.0f, 0.0f };
	QFont _font { "Verdana", 10 };
	math3d::vec4f _aabBox;
	ResultViewType _resultViewType = ResultViewType::Triangles;
	bool _editing = true;
	bool _panning = false;
	bool _initPan = true;
	bool _viewTraps = true;
	bool _outlineStart = true;
};
