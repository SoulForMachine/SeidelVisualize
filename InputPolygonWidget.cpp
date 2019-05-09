#include "InputPolygonWidget.h"
#include <QPainter>
#include "mathutil.h"


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

void InputPolygonWidget::ResetAndEdit()
{
	_points.clear();
	_editing = true;
}

const std::vector<math3d::vec2f>& InputPolygonWidget::GetPoints() const
{
	return _points;
}

void InputPolygonWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter { this };

	int w = width();
	int h = height();

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(QColor::fromRgb(120, 120, 120));

	math3d::vec2i axisLines[4] = {
		WorldToScreen({ -10.0f, 0.0f }),
		WorldToScreen({ 10.0f, 0.0f }),
		WorldToScreen({ 0.0f, -10.0f }),
		WorldToScreen({ 0.0f, 10.0f })
	};

	painter.drawLine(axisLines[0].x, axisLines[0].y, axisLines[1].x, axisLines[1].y);
	painter.drawLine(axisLines[2].x, axisLines[2].y, axisLines[3].x, axisLines[3].y);

	painter.setPen(QPen { Qt::GlobalColor::darkBlue });
	painter.setBrush(QBrush { Qt::GlobalColor::darkBlue });

	std::vector<QPoint> scrPts;
	scrPts.resize(_points.size());

	for (size_t i = 0; i < _points.size(); ++i)
	{
		auto scrPt = WorldToScreen({ _points[i].x, _points[i].y });
		scrPts[i] = { scrPt.x, scrPt.y };
		painter.drawEllipse(scrPts[i], 5, 5);
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
	switch (event->key())
	{
	case Qt::Key_Home:
	{
		_pan.set(width() / 2, height() / 2);
		_zoom = 1.0f;
		update();
		break;
	}
	}
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
