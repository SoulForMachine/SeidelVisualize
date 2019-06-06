#pragma once

#include <limits>
#include <set>
#include <QtWidgets/QMainWindow>
#include "ui_SeidelVisualize.h"

namespace Geometry
{
struct TriangulationState;
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

	void OnActionTrapToStart();
	void OnActionTrapToEnd();
	void OnActionTrapNextStep();
	void OnActionTrapPrevStep();

private:
	void TriangulateAndDisplay(const std::vector<math3d::vec2f>& points);
	void DumpTree(QTextStream& outStream);
	void DumpLog();

	Ui::SeidelVisualizeClass ui;
	Geometry::TriangulationState* _state = nullptr;
	size_t _dbgSteps = std::numeric_limits<size_t>::max();
};
