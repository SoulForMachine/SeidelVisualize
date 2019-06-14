#include "InputPolygonWidget.h"
#include <QPainter>
#include "mathutil.h"
#include "geometry.h"
#include "TriangulatePolygon_Seidel.h"


InputPolygonWidget::InputPolygonWidget(QWidget *parent)
	: QWidget(parent)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::FocusPolicy::ClickFocus);
}

InputPolygonWidget::~InputPolygonWidget()
{
}

void InputPolygonWidget::SetListener(InputPolygonWidgetListener* listener)
{
	_listener = listener;
}

void InputPolygonWidget::SetTreeState(Geometry::TriangulationState* state)
{
	if (!_editing)
	{
		_state = state;
		update();
	}
}

void InputPolygonWidget::ResetAndEdit()
{
	_outlines.clear();
	_editing = true;
	_state = nullptr;
	_outlineStart = true;
	ResetView();
}

void InputPolygonWidget::SetOutlines(const std::vector<std::vector<math3d::vec2f>>& outlines)
{
	_editing = false;
	_outlines = outlines;
	_state = nullptr;
	RecalcBBox();
	ResetView();
}

void InputPolygonWidget::SetOutlines(std::vector<std::vector<math3d::vec2f>>&& outlines)
{
	_editing = false;
	_outlines = std::move(outlines);
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

void InputPolygonWidget::SetViewTrapezoids(bool view)
{
	_viewTraps = view;
	update();
}

void InputPolygonWidget::SetResultViewType(ResultViewType type)
{
	_resultViewType = type;
	update();
}

void InputPolygonWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter { this };
	painter.setRenderHint(QPainter::Antialiasing);

	DrawAxes(painter);

	if (_editing)
	{
		DrawOutlines(painter);
		painter.drawEllipse({ _cursorPos.x, _cursorPos.y }, 5, 5);
	}
	else
	{
		// Draw trapezoid horizontal lines and numbers.
		if (_viewTraps)
			DrawTrapezoids(painter);

		switch (_resultViewType)
		{
		case ResultViewType::Triangles:
			DrawTriangles(painter);
			DrawOutlines(painter);
			break;

		case ResultViewType::MonotoneChains:
			DrawOutlines(painter);
			DrawMonotoneChains(painter);
			break;

		case ResultViewType::None:
			DrawOutlines(painter);
			break;
		}
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
			_outlineStart = false;
			update();
		}
		break;
	}

	case Qt::MouseButton::RightButton:
	{
		if (_editing)
		{
			if (_outlineStart && _outlines.size() > 0)
			{
				RecalcBBox();

				_editing = false;
				if (_listener)
					_listener->OnEditFinished();
				update();
			}
			else if (_points.size() > 2)
			{
				_outlineStart = true;
				_outlines.push_back(std::move(_points));
				update();
			}
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
	if (event->key() == Qt::Key_Escape && _editing)
	{
		// Reset currently edited outline.
		_points.clear();
		_outlineStart = true;
		update();
		event->accept();
		return;
	}

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

void InputPolygonWidget::DrawAxes(QPainter& painter)
{
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
}

void InputPolygonWidget::DrawOutlines(QPainter& painter)
{
	// Draw the polygon: segments and points marked with indices.

	painter.setPen(QPen { Qt::GlobalColor::darkBlue });
	painter.setBrush(QBrush { Qt::GlobalColor::darkBlue });

	size_t ptInd = 0;
	std::vector<QPoint> scrPts;

	for (size_t i = 0; i < _outlines.size(); ++i)
	{
		for (size_t j = 0; j < _outlines[i].size(); ++j)
		{
			auto scrPt = WorldToScreen({ _outlines[i][j].x, _outlines[i][j].y });
			scrPts.push_back({ scrPt.x, scrPt.y });
			painter.drawEllipse(scrPts[j], 5, 5);
			painter.drawText(QPoint { scrPts[j].x() - 16, scrPts[j].y() }, QString::number(ptInd++));
		}

		scrPts.push_back(scrPts.front());

		painter.drawPolyline(scrPts.data(), scrPts.size());
		scrPts.clear();
	}

	for (auto& pt : _points)
	{
		auto scrPt = WorldToScreen({ pt.x, pt.y });
		scrPts.push_back({ scrPt.x, scrPt.y });
		painter.drawEllipse(scrPts.back(), 5, 5);
		painter.drawText(QPoint { scrPt.x - 16, scrPt.y }, QString::number(ptInd++));
	}
	
	scrPts.push_back({ _cursorPos.x, _cursorPos.y });
	painter.drawPolyline(scrPts.data(), scrPts.size());
	scrPts.clear();
}

void InputPolygonWidget::DrawTrapezoids(QPainter& painter)
{
	if (_state == nullptr)
		return;

	for (auto trap : _state->trapezoids)
	{
		// Draw trapezoid numbers.
		math3d::vec2f extend { 0.25f, 0.25f }; //{ (_aabBox.z - _aabBox.x) * 0.2f, (_aabBox.w - _aabBox.y) * 0.2f };

		float up = 0, down = 0;
		math3d::vec2f position { 0.0f, 0.0f };

		if (trap->upperPointIndex >= 0)
			up = _state->pointCoords[trap->upperPointIndex].y;
		else
			up = _state->pointCoords[trap->lowerPointIndex].y + extend.y;

		if (trap->lowerPointIndex >= 0)
			down = _state->pointCoords[trap->lowerPointIndex].y;
		else
			down = _state->pointCoords[trap->upperPointIndex].y - extend.y;

		math3d::vec3f upperLine = math3d::line_from_points_2d(math3d::vec2f { 0.0f, up }, math3d::vec2f { 1.0f, up });
		math3d::vec3f lowerLine = math3d::line_from_points_2d(math3d::vec2f { 0.0f, down }, math3d::vec2f { 1.0f, down });
		math3d::vec2f midLeft, midRight;

		if (trap->leftSegmentIndex >= 0)
		{
			math3d::vec2f upLeft, lowLeft;
			auto& segment = _state->segments[trap->leftSegmentIndex];
			if (math3d::intersect_lines_2d(upLeft, segment.line, upperLine) &&
				math3d::intersect_lines_2d(lowLeft, segment.line, lowerLine))
			{
				midLeft = (upLeft + lowLeft) / 2.0f;
			}
			else
			{
				midLeft = math3d::min(_state->pointCoords[segment.lowerPointIndex], _state->pointCoords[segment.upperPointIndex]);
			}
		}

		if (trap->rightSegmentIndex >= 0)
		{
			math3d::vec2f upRight, lowRight;
			auto& segment = _state->segments[trap->rightSegmentIndex];
			if (math3d::intersect_lines_2d(upRight, segment.line, upperLine) &&
				math3d::intersect_lines_2d(lowRight, segment.line, lowerLine))
			{
				midRight = (upRight + lowRight) / 2.0f;
			}
			else
			{
				midRight = math3d::max(_state->pointCoords[segment.lowerPointIndex], _state->pointCoords[segment.upperPointIndex]);
			}
		}

		if (trap->leftSegmentIndex >= 0 && trap->rightSegmentIndex >= 0)
		{
			position = (midLeft + midRight) / 2.0f;
		}
		else if (trap->leftSegmentIndex < 0 && trap->rightSegmentIndex >= 0)
		{
			position.set(midRight.x - extend.x, midRight.y);
		}
		else if (trap->leftSegmentIndex >= 0 && trap->rightSegmentIndex < 0)
		{
			position.set(midLeft.x + extend.x, midLeft.y);
		}
		else
		{
			position.x = (_aabBox.x + _aabBox.z) / 2.0f;

			if (trap->upperPointIndex >= 0)
				position.y = _state->pointCoords[trap->upperPointIndex].y - extend.y;
			else if (trap->lowerPointIndex >= 0)
				position.y = _state->pointCoords[trap->lowerPointIndex].y + extend.y;
		}

		auto scrPos = WorldToScreen(position);

		painter.setPen(QPen { (trap->status == Geometry::Trapezoid::Status::Inside) ? Qt::GlobalColor::darkGreen : Qt::GlobalColor::red });
		auto strTrapNum = QString::number(trap->number);
		QFontMetrics fontMetrics { _font, painter.device() };
		auto textSize = fontMetrics.size(Qt::TextSingleLine, strTrapNum);
		QRect textRect {
			scrPos.x - textSize.width() / 2,
			scrPos.y - textSize.height() / 2,
			textSize.width(), textSize.height()
		};
		painter.drawText(textRect, strTrapNum);

		// Draw horizontal lines.
		if (trap->upperPointIndex >= 0)
		{
			math3d::vec2f wUpperPt = _state->pointCoords[trap->upperPointIndex];
			auto horizLine = math3d::line_from_points_2d(wUpperPt, wUpperPt + math3d::vec2f { 1.0f, 0.0f });
			math3d::vec2f wLeftPt { _aabBox.x - extend.x, wUpperPt.y };
			math3d::vec2f wRightPt { _aabBox.z + extend.x, wUpperPt.y };

			if (trap->leftSegmentIndex >= 0)
			{
				auto& segment = _state->segments[trap->leftSegmentIndex];
				math3d::intersect_lines_2d(wLeftPt, segment.line, horizLine);
			}

			if (trap->rightSegmentIndex >= 0)
			{
				auto& segment = _state->segments[trap->rightSegmentIndex];
				math3d::intersect_lines_2d(wRightPt, segment.line, horizLine);
			}

			auto sLeftPt = WorldToScreen(wLeftPt);
			auto sRightPt = WorldToScreen(wRightPt);
			painter.setPen(QPen { Qt::darkGreen, 1, Qt::PenStyle::DashLine });
			painter.drawLine(sLeftPt.x, sLeftPt.y, sRightPt.x, sRightPt.y);
		}
	}
}

void InputPolygonWidget::DrawTriangles(QPainter& painter)
{
	if (_state == nullptr)
		return;

	painter.setPen(QPen { Qt::cyan });

	for (size_t i = 0; i < _state->outIndices.size(); i += 3)
	{
		std::vector<QPoint> points;

		for (size_t j = 0; j < 4; ++j)
		{
			auto scrPt = WorldToScreen(_state->pointCoords[_state->outIndices[i + j % 3]]);
			points.push_back(QPoint { scrPt.x, scrPt.y });
		}

		painter.drawPolyline(points.data(), static_cast<int>(points.size()));
	}
}

void InputPolygonWidget::DrawMonotoneChains(QPainter& painter)
{
	if (_state == nullptr)
		return;

	painter.setPen(QPen { Qt::cyan });

	for (auto& chain : _state->monChains)
	{
		std::vector<QPoint> points;

		for (auto ptInd : chain)
		{
			auto scrPt = WorldToScreen(_state->pointCoords[ptInd]);
			points.push_back(QPoint { scrPt.x, scrPt.y });
		}

		painter.drawPolyline(points.data(), static_cast<int>(points.size()));
	}
}

void InputPolygonWidget::RecalcBBox()
{
	_aabBox.x = _outlines[0][0].x;
	_aabBox.y = _outlines[0][0].y;
	_aabBox.z = _outlines[0][0].x;
	_aabBox.w = _outlines[0][0].y;

	for (auto& outline : _outlines)
	{
		for (auto& pt : outline)
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
}
