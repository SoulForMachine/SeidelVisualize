#include "InputPolygonWidget.h"
#include <QPainter>
#include "mathutil.h"
#include "geometry.h"
#include "TriangulatePolygon_Seidel.h"


InputPolygonWidget::InputPolygonWidget(QWidget *parent)
	: QWidget(parent)
{
	setMouseTracking(true);
}

InputPolygonWidget::~InputPolygonWidget()
{
}

void InputPolygonWidget::SetListener(InputPolygonWidgetListener* listener)
{
	_listener = listener;
}

void InputPolygonWidget::SetTreeState(Geometry::TrapezoidTreeState* state)
{
	if (!_editing)
	{
		_state = state;
		update();
	}
}

void InputPolygonWidget::ResetAndEdit()
{
	_points.clear();
	_editing = true;
	_state = nullptr;
	ResetView();
}

const std::vector<math3d::vec2f>& InputPolygonWidget::GetPoints() const
{
	return _points;
}

void InputPolygonWidget::SetPolygon(const std::vector<math3d::vec2f>& points)
{
	_editing = false;
	_points = points;
	_state = nullptr;
	RecalcBBox();
	ResetView();
}

void InputPolygonWidget::ResetView()
{
	_pan.set(width() / 2, height() / 2);
	_zoom = 1.0f;
	update();
}

void InputPolygonWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter { this };

	int w = width();
	int h = height();

	painter.setRenderHint(QPainter::Antialiasing);

	// Draw the axes.
	
	painter.setPen(QColor::fromRgb(120, 120, 120));

	math3d::vec2i axisLines[4] = {
		WorldToScreen({ -10.0f, 0.0f }),
		WorldToScreen({ 10.0f, 0.0f }),
		WorldToScreen({ 0.0f, -10.0f }),
		WorldToScreen({ 0.0f, 10.0f })
	};

	painter.drawLine(axisLines[0].x, axisLines[0].y, axisLines[1].x, axisLines[1].y);
	painter.drawLine(axisLines[2].x, axisLines[2].y, axisLines[3].x, axisLines[3].y);

	// Draw the polygon: segments and points marked with indices.

	painter.setPen(QPen { Qt::GlobalColor::darkBlue });
	painter.setBrush(QBrush { Qt::GlobalColor::darkBlue });

	std::vector<QPoint> scrPts;
	scrPts.resize(_points.size());

	for (size_t i = 0; i < _points.size(); ++i)
	{
		auto scrPt = WorldToScreen({ _points[i].x, _points[i].y });
		scrPts[i] = { scrPt.x, scrPt.y };
		painter.drawEllipse(scrPts[i], 5, 5);
		painter.drawText(QPoint { scrPts[i].x() - 16, scrPts[i].y() }, QString::number(i));
	}

	if (_editing)
		scrPts.push_back({ _cursorPos.x, _cursorPos.y });
	else
		scrPts.push_back(scrPts.front());

	painter.drawPolyline(scrPts.data(), scrPts.size());

	if (_editing)
	{
		painter.drawEllipse({ _cursorPos.x, _cursorPos.y }, 5, 5);
	}
	else
	{
		// Draw trapezoid horizontal lines and numbers.
		if (_state != nullptr)
			DrawTrapezoids(painter, _state->treeRootNode);
	}
}

void InputPolygonWidget::resizeEvent(QResizeEvent* event)
{
	if (_initPan)
		_pan.set(width() / 2, height() / 2);
}

void InputPolygonWidget::mousePressEvent(QMouseEvent* event)
{
	switch (event->button())
	{
	case Qt::MouseButton::LeftButton:
	{
		if (_editing)
		{
			_points.push_back(ScreenToWorld({ event->x(), event->y() }));
			update();
		}
		break;
	}

	case Qt::MouseButton::RightButton:
	{
		if (_editing && _points.size() > 2)
		{
			RecalcBBox();

			_editing = false;
			if (_listener)
				_listener->OnEditFinished();
			update();
		}
		break;
	}
	case Qt::MouseButton::MiddleButton:
	{
		_panning = true;
		_initPan = false;
		_prevPanPos.set(event->x(), event->y());
		break;
	}
	}
}

void InputPolygonWidget::mouseReleaseEvent(QMouseEvent* event)
{
	switch (event->button())
	{
	case Qt::MouseButton::MiddleButton:
	{
		_panning = false;
		break;
	}
	}
}

void InputPolygonWidget::mouseMoveEvent(QMouseEvent* event)
{
	_cursorPos.set(event->x(), event->y());

	if (_panning)
	{
		_pan += _cursorPos - _prevPanPos;
		_prevPanPos = _cursorPos;
	}

	if (_editing || _panning)
		update();
}

void InputPolygonWidget::wheelEvent(QWheelEvent* event)
{
	_zoom += event->angleDelta().y() / 120 * 0.05f;
	_zoom = math3d::clamp(_zoom, 0.05f, 5.0f);
	update();
}

void InputPolygonWidget::keyPressEvent(QKeyEvent* event)
{
	QWidget::keyPressEvent(event);
}

math3d::vec2f InputPolygonWidget::ScreenToWorld(const math3d::vec2i& screenPt)
{
	auto panned = math3d::vec2f { (float)screenPt.x, (float)height() - screenPt.y } - math3d::vec2f { (float)_pan.x, (float)height() - _pan.y };

	return panned / (PIXELS_PER_METER * _zoom);
}

math3d::vec2i InputPolygonWidget::WorldToScreen(const math3d::vec2f& worldPt)
{
	auto scrPt = math3d::vec2i { _pan.x, height() - _pan.y } + math3d::vec2i { worldPt * PIXELS_PER_METER * _zoom };
	scrPt.y = height() - scrPt.y;
	return scrPt;
}

void InputPolygonWidget::DrawTrapezoids(QPainter& painter, Geometry::TrapezoidationTreeNode* node)
{
	if (node->type == Geometry::TrapezoidationTreeNode::Type::TRAPEZOID)
	{
		// Draw trapezoid numbers.
		auto trap = node->trapezoid;
		math3d::vec2f extend { (_aabBox.z - _aabBox.x) * 0.2f, (_aabBox.w - _aabBox.y) * 0.2f };

		float up = 0, down = 0, left = 0, right = 0;

		if (trap->upperVertexIndex >= 0)
			up = _state->pointCoords[trap->upperVertexIndex].y;
		else
			up = _state->pointCoords[trap->lowerVertexIndex].y + extend.y;

		if (trap->lowerVertexIndex >= 0)
			down = _state->pointCoords[trap->lowerVertexIndex].y;
		else
			down = _state->pointCoords[trap->upperVertexIndex].y - extend.y;

		math3d::vec3f upperLine = math3d::line_from_points_2d(math3d::vec2f { 0.0f, up }, math3d::vec2f { 1.0f, up });
		math3d::vec3f lowerLine = math3d::line_from_points_2d(math3d::vec2f { 0.0f, down }, math3d::vec2f { 1.0f, down });

		if (trap->leftSegmentIndex >= 0)
		{
			math3d::vec2f upLeft, lowLeft;
			auto& segment = _state->segments[trap->leftSegmentIndex];
			math3d::intersect_lines_2d(upLeft, segment.line, upperLine);
			math3d::intersect_lines_2d(lowLeft, segment.line, lowerLine);
			left = std::min(upLeft.x, lowLeft.x);

			/*auto ul = WorldToScreen(upLeft);
			auto ll = WorldToScreen(lowLeft);
			painter.drawEllipse({ ul.x, ul.y }, 5, 5);
			painter.drawEllipse({ ll.x, ll.y }, 5, 5);*/
		}

		if (trap->rightSegmentIndex >= 0)
		{
			math3d::vec2f upRight, lowRight;
			auto& segment = _state->segments[trap->rightSegmentIndex];
			math3d::intersect_lines_2d(upRight, segment.line, upperLine);
			math3d::intersect_lines_2d(lowRight, segment.line, lowerLine);
			right = std::max(upRight.x, lowRight.x);

			/*auto ur = WorldToScreen(upRight);
			auto lr = WorldToScreen(lowRight);
			painter.drawEllipse({ ur.x, ur.y }, 5, 5);
			painter.drawEllipse({ lr.x, lr.y }, 5, 5);*/
		}

		if (left == right)
		{
			float mid = (_aabBox.z + _aabBox.x) / 2.0f;
			left = mid - extend.x;
			right = mid + extend.x;
		}

		if (up == down)
		{
			float mid = (_aabBox.w + _aabBox.y) / 2.0f;
			up = mid + extend.y;
			down = mid - extend.y;
		}

		math3d::vec4f wRect { left, up, right, down };
		auto sLeftUp = WorldToScreen(math3d::vec2f { left, up });
		auto sRightDown = WorldToScreen(math3d::vec2f { right, down });
		QRect sRect {
			QPoint { sLeftUp.x, sLeftUp.y },
			QPoint { sRightDown.x, sRightDown.y}
		};

		painter.setPen(QPen { Qt::GlobalColor::red });
		painter.drawText(sRect, QString::number(trap->number), QTextOption { Qt::AlignHCenter | Qt::AlignVCenter });

		// Draw horizontal lines.
		if (trap->upperVertexIndex >= 0)
		{
			math3d::vec2f wUpperPt = _state->pointCoords[trap->upperVertexIndex];
			auto horizLine = math3d::line_from_points_2d(wUpperPt, wUpperPt + math3d::vec2f { 1.0f, 0.0f });
			math3d::vec2f wLeftPt { _aabBox.x - extend.x, wUpperPt.y };
			math3d::vec2f wRightPt { _aabBox.z + extend.x, wUpperPt.y };

			if (trap->leftSegmentIndex >= 0)
			{
				auto& segment = _state->segments[trap->leftSegmentIndex];
				if (math3d::intersect_lines_2d(wLeftPt, segment.line, horizLine))
				{

				}
			}

			if (trap->rightSegmentIndex >= 0)
			{
				auto& segment = _state->segments[trap->rightSegmentIndex];
				if (math3d::intersect_lines_2d(wRightPt, segment.line, horizLine))
				{

				}
			}

			auto sLeftPt = WorldToScreen(wLeftPt);
			auto sRightPt = WorldToScreen(wRightPt);
			painter.setPen(QPen { Qt::darkGreen, 1, Qt::PenStyle::DashLine });
			painter.drawLine(sLeftPt.x, sLeftPt.y, sRightPt.x, sRightPt.y);
		}
	}
	else
	{
		DrawTrapezoids(painter, node->left);
		DrawTrapezoids(painter, node->right);
	}
}

void InputPolygonWidget::RecalcBBox()
{
	_aabBox.x = _points[0].x;
	_aabBox.y = _points[0].y;
	_aabBox.z = _points[0].x;
	_aabBox.w = _points[0].y;

	for (auto& pt : _points)
	{
		if (pt.x < _aabBox.x)
			_aabBox.x = pt.x;
		if (pt.x > _aabBox.z)
			_aabBox.z = pt.x;

		if (pt.y < _aabBox.y)
			_aabBox.y = pt.y;
		if (pt.y > _aabBox.w)
			_aabBox.w = pt.y;
	}
}
