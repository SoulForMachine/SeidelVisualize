#include "TrapezoidationTreeWidget.h"
#include <QPainter>

TrapezoidationTreeWidget::TrapezoidationTreeWidget(QWidget *parent)
	: QWidget(parent)
{
}

TrapezoidationTreeWidget::~TrapezoidationTreeWidget()
{
}

void TrapezoidationTreeWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter { this };

	
}
