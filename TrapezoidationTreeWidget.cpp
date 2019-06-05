#include "TrapezoidationTreeWidget.h"
#include <algorithm>
#include <QPainter>
#include <QMouseEvent>
#include "TriangulatePolygon_Seidel.h"
#include "mathutil.h"


TrapezoidationTreeWidget::TrapezoidationTreeWidget(QWidget *parent)
	: QWidget(parent)
{
}

TrapezoidationTreeWidget::~TrapezoidationTreeWidget()
{
}

void TrapezoidationTreeWidget::SetTreeState(Geometry::TrapezoidTreeState* state)
{
	_state = state;
	//_numTreeLevels = (state != nullptr) ? GetTreeDepth(state->treeRootNode) : 0;
	if (state != nullptr)
		UpdateNodeWeights(state->treeRootNode);
		
	update();
}

void TrapezoidationTreeWidget::ResetView()
{
	_zoom = 1.0f;
	_pan = { 0, 0 };
	update();
}

void TrapezoidationTreeWidget::paintEvent(QPaintEvent* event)
{
	if (_state == nullptr)
	{
		return;
	}

	QPainter painter { this };

	painter.setPen(QPen { Qt::GlobalColor::black });
	painter.setBrush(QBrush { Qt::GlobalColor::white });
	painter.setFont(_font);
	painter.setRenderHint(QPainter::Antialiasing);

	auto w = width();
	auto h = height();

	painter.save();
	painter.translate(w / 2, h / 2);
	painter.scale(_zoom, _zoom);
	painter.translate(-w / 2, -h / 2);
	painter.translate(_pan);

	QPoint rootPos { width() / 2, 100 };
	DrawNode(painter, _state->treeRootNode, rootPos, 1);

	painter.restore();
}

void TrapezoidationTreeWidget::mousePressEvent(QMouseEvent* event)
{
	switch (event->button())
	{
	case Qt::MouseButton::MiddleButton:
	{
		_panning = true;
		_prevPanPos = event->pos();
		break;
	}
	}
}

void TrapezoidationTreeWidget::mouseReleaseEvent(QMouseEvent* event)
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

void TrapezoidationTreeWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (_panning)
	{
		_pan += event->pos() - _prevPanPos;
		_prevPanPos = event->pos();
		update();
	}
}

void TrapezoidationTreeWidget::wheelEvent(QWheelEvent* event)
{
	_zoom += event->angleDelta().y() / 120 * 0.1f;
	_zoom = math3d::clamp(_zoom, 0.05f, 5.0f);
	update();
}

void TrapezoidationTreeWidget::UpdateNodeWeights(Geometry::TrapezoidationTreeNode* node)
{
	if (node == nullptr)
		return;

	if (node->left != nullptr && node->right != nullptr)
	{
		UpdateNodeWeights(node->left);
		UpdateNodeWeights(node->right);
		node->leftWeight = node->left->leftWeight + node->left->rightWeight + 1;
		node->rightWeight = node->right->leftWeight + node->right->rightWeight + 1;
	}
	else
	{
		node->leftWeight = 0;
		node->rightWeight = 0;
	}
}

int TrapezoidationTreeWidget::GetTreeDepth(Geometry::TrapezoidationTreeNode* rootNode)
{
	if (rootNode == nullptr) return 0;
	return std::max(GetTreeDepth(rootNode->left), GetTreeDepth(rootNode->right)) + 1;
}

void TrapezoidationTreeWidget::DrawNode(QPainter& painter, Geometry::TrapezoidationTreeNode* node, const QPoint& position, int level)
{
	if (node == nullptr)
		return;

	QPoint leftPos, rightPos;

	if (node->left)
	{
		unsigned int steps = 1 + node->left->rightWeight;
		leftPos.setX(position.x() - HORIZONTAL_NODE_STEP * steps);
		leftPos.setY(position.y() + VERTICAL_NODE_STEP);

		painter.drawLine(position, leftPos);
	}

	if (node->right)
	{
		unsigned int steps = 1 + node->right->leftWeight;
		rightPos.setX(position.x() + HORIZONTAL_NODE_STEP * steps);
		rightPos.setY(position.y() + VERTICAL_NODE_STEP);

		painter.drawLine(position, rightPos);
	}

	switch (node->type)
	{
	case Geometry::TrapezoidationTreeNode::Type::VERTEX:
	{
		QPoint halfSize { 20, 20 };
		QRect rect { position - halfSize, position + halfSize };
		painter.drawRect(rect);
		painter.drawText(rect, QString::number(node->elementIndex), QTextOption { Qt::AlignHCenter | Qt::AlignVCenter });
		break;
	}

	case Geometry::TrapezoidationTreeNode::Type::SEGMENT:
	{
		QPoint halfSize { 30, 20 };
		QRect rect { position - halfSize, position + halfSize };
		painter.drawRoundedRect(rect, 10.0f, 10.0f);

		QString text =
			QString::number(_state->segments[node->elementIndex].upperPointIndex) + ", " +
			QString::number(_state->segments[node->elementIndex].lowerPointIndex);

		painter.drawText(rect, text, QTextOption { Qt::AlignHCenter | Qt::AlignVCenter });
		break;
	}

	case Geometry::TrapezoidationTreeNode::Type::TRAPEZOID:
	{
		QPoint halfSize { 20, 20 };
		QRect rect { position - halfSize, position + halfSize };
		painter.drawEllipse(rect);
		painter.drawText(rect, QString::number(node->trapezoid->number), QTextOption { Qt::AlignHCenter | Qt::AlignVCenter });
		break;
	}
	}

	/*{
		QPoint halfSize { 40, 20 };
		QRect rect { position - halfSize, position + halfSize + QPoint { 0, 80 } };

		painter.drawText(rect, "l: " + QString::number(node->leftWeight) + " r: " + QString::number(node->rightWeight), QTextOption { Qt::AlignHCenter | Qt::AlignVCenter });
	}*/

	if (node->left)
		DrawNode(painter, node->left, leftPos, level + 1);

	if (node->right)
		DrawNode(painter, node->right, rightPos, level + 1);
}
