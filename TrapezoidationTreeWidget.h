#pragma once

#include <QWidget>

class TrapezoidationTreeWidget : public QWidget
{
	Q_OBJECT

public:
	TrapezoidationTreeWidget(QWidget *parent);
	~TrapezoidationTreeWidget();

protected:
	virtual void paintEvent(QPaintEvent* event) override;
};
