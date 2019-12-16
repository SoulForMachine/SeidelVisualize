#pragma once

#include <limits>
#include <vector>
#include <QtWidgets/QMainWindow>
#include "ui_SeidelVisualize.h"
#include "TriangulatePolygon_Seidel.h"


class SeidelVisualize : public QMainWindow, public InputPolygonWidgetListener
{
	Q_OBJECT

public:
	SeidelVisualize(QWidget *parent = Q_NULLPTR);
	~SeidelVisualize();

	virtual void OnEditFinished() override;

	static void Benchmark(const QString& polyFile, int numIters);

private slots:

	void OnActionLoad();
	void OnActionSave();
	void OnActionReset();
	void OnActionResetView();

	void OnActionTrapToStart();
	void OnActionTrapToEnd();
	void OnActionTrapNextStep();
	void OnActionTrapPrevStep();

	void OnActionViewFillPolygons();
	void OnActionViewTrapezoids();
	void OnActionViewResult(QAction* action);
	void OnActionOptionsRandSeg();
	void OnActionOptionsTrisWinding(QAction* action);
	void OnActionOptionsFillRule(QAction* action);

private:
	void TriangulateAndDisplay();
	void DumpTraps(QTextStream& outStream);
	void DumpMonChains(QTextStream& outStream);
	void DumpTris(QTextStream& outStream);
	void DumpLog();
	static bool LoadPolyFile(const QString& polyFile, std::vector<std::vector<math3d::vec2f>>& outlines);
	static bool SavePolyFile(const QString& polyFile, const std::vector<std::vector<math3d::vec2f>>& outlines);

	Ui::SeidelVisualizeClass ui;
	Geometry::TriangulationState* _state = nullptr;
	int_t _dbgSteps = std::numeric_limits<int_t>::max();
	Geometry::Winding _triangleWinding = Geometry::Winding::CCW;
	Geometry::FillRule _fillRule = Geometry::FillRule::ODD;
	bool _randSegments = false;
};
