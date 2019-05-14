#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_SeidelVisualize.h"

namespace Geometry
{
struct TrapezoidTreeState;
}


class SeidelVisualize : public QMainWindow, public InputPolygonWidgetListener
{
	Q_OBJECT

public:
	SeidelVisualize(QWidget *parent = Q_NULLPTR);
	~SeidelVisualize();

	virtual void OnEditFinished() override;

private slots:

	void OnActionLoad();
	void OnActionSave();
	void OnActionReset();
	void OnActionResetView();

private:
	void TriangulateAndDisplay(const std::vector<math3d::vec2f>& points);
	void DumpLog();

	Ui::SeidelVisualizeClass ui;
	Geometry::TrapezoidTreeState* _state = nullptr;
};
