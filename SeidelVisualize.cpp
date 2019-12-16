#include "SeidelVisualize.h"
#include <chrono>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>


SeidelVisualize::SeidelVisualize(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	showMaximized();

	ui.widgetInputPolygon->SetListener(this);

	auto actionGroupResults = new QActionGroup { this };
	actionGroupResults->addAction(ui.actionViewMonotoneChains);
	actionGroupResults->addAction(ui.actionViewTriangles);
	actionGroupResults->addAction(ui.actionViewNone);
	
	switch (ui.widgetInputPolygon->GetResultViewType())
	{
	case InputPolygonWidget::ResultViewType::Triangles:
		ui.actionViewTriangles->setChecked(true);
		break;

	case InputPolygonWidget::ResultViewType::MonotoneChains:
		ui.actionViewMonotoneChains->setChecked(true);
		break;

	case InputPolygonWidget::ResultViewType::None:
		ui.actionViewNone->setChecked(true);
		break;
	}

	auto actionGroupTrisWind = new QActionGroup { this };
	actionGroupTrisWind->addAction(ui.actionOptionsTrisWindCCW);
	actionGroupTrisWind->addAction(ui.actionOptionsTrisWindCW);

	switch (_triangleWinding)
	{
	case Geometry::Winding::CCW:
		ui.actionOptionsTrisWindCCW->setChecked(true);
		break;

	case Geometry::Winding::CW:
		ui.actionOptionsTrisWindCW->setChecked(true);
		break;
	}

	auto actionGroupFillRule = new QActionGroup { this };
	actionGroupFillRule->addAction(ui.actionOptionsFillRuleNonZero);
	actionGroupFillRule->addAction(ui.actionOptionsFillRuleOdd);

	switch (_fillRule)
	{
	case Geometry::FillRule::NON_ZERO:
		ui.actionOptionsFillRuleNonZero->setChecked(true);
		break;

	case Geometry::FillRule::ODD:
		ui.actionOptionsFillRuleOdd->setChecked(true);
		break;
	}

	ui.actionViewFillPolygons->setChecked(ui.widgetInputPolygon->GetFillPolygons());
	ui.actionViewTrapezoids->setChecked(ui.widgetInputPolygon->GetViewTrapezoids());
	ui.actionOptionsRandomizeSegments->setChecked(_randSegments);

	connect(ui.actionLoad, &QAction::triggered, this, &SeidelVisualize::OnActionLoad);
	connect(ui.actionSave, &QAction::triggered, this, &SeidelVisualize::OnActionSave);
	connect(ui.actionReset, &QAction::triggered, this, &SeidelVisualize::OnActionReset);
	connect(ui.actionResetViews, &QAction::triggered, this, &SeidelVisualize::OnActionResetView);

	connect(ui.actionTrapToStart, &QAction::triggered, this, &SeidelVisualize::OnActionTrapToStart);
	connect(ui.actionTrapToEnd, &QAction::triggered, this, &SeidelVisualize::OnActionTrapToEnd);
	connect(ui.actionTrapNextStep, &QAction::triggered, this, &SeidelVisualize::OnActionTrapNextStep);
	connect(ui.actionTrapPreviousStep, &QAction::triggered, this, &SeidelVisualize::OnActionTrapPrevStep);

	connect(ui.actionViewFillPolygons, &QAction::triggered, this, &SeidelVisualize::OnActionViewFillPolygons);
	connect(ui.actionViewTrapezoids, &QAction::triggered, this, &SeidelVisualize::OnActionViewTrapezoids);
	connect(ui.actionOptionsRandomizeSegments, &QAction::triggered, this, &SeidelVisualize::OnActionOptionsRandSeg);
	connect(actionGroupResults, &QActionGroup::triggered, this, &SeidelVisualize::OnActionViewResult);
	connect(actionGroupTrisWind, &QActionGroup::triggered, this, &SeidelVisualize::OnActionOptionsTrisWinding);
	connect(actionGroupFillRule, &QActionGroup::triggered, this, &SeidelVisualize::OnActionOptionsFillRule);
}

SeidelVisualize::~SeidelVisualize()
{
	delete _state;
}

void SeidelVisualize::OnEditFinished()
{
	_dbgSteps = std::numeric_limits<int_t>::max();
	TriangulateAndDisplay();
}

void SeidelVisualize::Benchmark(const QString& polyFile, int numIters)
{
	std::vector<std::vector<math3d::vec2f>> outlines;
	if (LoadPolyFile(polyFile, outlines))
	{
		std::chrono::high_resolution_clock hiResClk;
		auto startTime = hiResClk.now();

		for (int i = 0; i < numIters; ++i)
		{
			Geometry::TriangulationState state { outlines };
			Geometry::TriangulatePolygon_Seidel(state);
		}

		auto endTime = hiResClk.now();
		auto duration = endTime - startTime;
		printf("Completed in %f s\n", duration.count() / 1'000'000'000.0);
	}
}

void SeidelVisualize::OnActionLoad()
{
	std::vector<std::vector<math3d::vec2f>> outlines;

	auto fileName = QFileDialog::getOpenFileName(this, "Load polygon", "", "Polygon files (*.poly)");

	if (!fileName.isEmpty())
	{
		if (LoadPolyFile(fileName, outlines))
		{
			if (!outlines.empty())
			{
				ui.widgetInputPolygon->SetOutlines(std::move(outlines));
				_dbgSteps = std::numeric_limits<int_t>::max();
				TriangulateAndDisplay();
			}
		}
	}
}

void SeidelVisualize::OnActionSave()
{
	if (!ui.widgetInputPolygon->IsEditing())
	{
		const auto& outlines = ui.widgetInputPolygon->GetOutlines();

		auto fileName = QFileDialog::getSaveFileName(this, "Save polygon", "", "Polygon files (*.poly)");

		if (!fileName.isEmpty())
		{
			SavePolyFile(fileName, outlines);
		}
	}
}

void SeidelVisualize::OnActionReset()
{
	ui.widgetInputPolygon->ResetAndEdit();
	ui.widgetTrapTree->SetTreeState(nullptr);
	delete _state;
	_state = nullptr;
	_dbgSteps = std::numeric_limits<int_t>::max();
}

void SeidelVisualize::OnActionResetView()
{
	ui.widgetInputPolygon->ResetView();
	ui.widgetTrapTree->ResetView();
}

void SeidelVisualize::OnActionTrapToStart()
{
	if (!_randSegments)
	{
		_dbgSteps = 0;
		TriangulateAndDisplay();
	}
}

void SeidelVisualize::OnActionTrapToEnd()
{
	if (!_randSegments)
	{
		_dbgSteps = std::numeric_limits<int_t>::max();
		TriangulateAndDisplay();
	}
}

void SeidelVisualize::OnActionTrapNextStep()
{
	if (!ui.widgetInputPolygon->IsEditing() && !_randSegments)
	{
		if (_dbgSteps < std::numeric_limits<int_t>::max())
		{
			_dbgSteps += 1;
			TriangulateAndDisplay();
		}
	}
}

void SeidelVisualize::OnActionTrapPrevStep()
{
	if (!ui.widgetInputPolygon->IsEditing() && !_randSegments)
	{
		if (_dbgSteps > 0)
		{
			_dbgSteps -= 1;
			TriangulateAndDisplay();
		}
	}
}

void SeidelVisualize::OnActionViewFillPolygons()
{
	ui.widgetInputPolygon->SetFillPolygons(!ui.widgetInputPolygon->GetFillPolygons());
}

void SeidelVisualize::OnActionViewTrapezoids()
{
	ui.widgetInputPolygon->SetViewTrapezoids(!ui.widgetInputPolygon->GetViewTrapezoids());
}

void SeidelVisualize::OnActionViewResult(QAction* action)
{
	if (action == ui.actionViewTriangles)
		ui.widgetInputPolygon->SetResultViewType(InputPolygonWidget::ResultViewType::Triangles);
	else if (action == ui.actionViewMonotoneChains)
		ui.widgetInputPolygon->SetResultViewType(InputPolygonWidget::ResultViewType::MonotoneChains);
	else if (action == ui.actionViewNone)
		ui.widgetInputPolygon->SetResultViewType(InputPolygonWidget::ResultViewType::None);
}

void SeidelVisualize::OnActionOptionsRandSeg()
{
	_randSegments = !_randSegments;
	_dbgSteps = std::numeric_limits<int_t>::max();
	TriangulateAndDisplay();
}

void SeidelVisualize::OnActionOptionsTrisWinding(QAction* action)
{
	if (action == ui.actionOptionsTrisWindCCW)
		_triangleWinding = Geometry::Winding::CCW;
	else if (action == ui.actionOptionsTrisWindCW)
		_triangleWinding = Geometry::Winding::CW;

	_dbgSteps = std::numeric_limits<int_t>::max();
	TriangulateAndDisplay();
}

void SeidelVisualize::OnActionOptionsFillRule(QAction* action)
{
	if (action == ui.actionOptionsFillRuleNonZero)
		_fillRule = Geometry::FillRule::NON_ZERO;
	else if (action == ui.actionOptionsFillRuleOdd)
		_fillRule = Geometry::FillRule::ODD;

	_dbgSteps = std::numeric_limits<int_t>::max();
	TriangulateAndDisplay();
}

void SeidelVisualize::TriangulateAndDisplay()
{
	if (ui.widgetInputPolygon->IsEditing())
		return;

	delete _state;
	_state = new Geometry::TriangulationState { ui.widgetInputPolygon->GetOutlines() };
	_state->randomizeSegments = false;
	_state->dbgSteps = _dbgSteps;
	_state->triangleWinding = _triangleWinding;
	_state->randomizeSegments = _randSegments;
	_state->fillRule = _fillRule;

	Geometry::TriangulatePolygon_Seidel(*_state);

	_dbgSteps -= _state->dbgSteps;

	ui.widgetTrapTree->SetTreeState(_state);
	ui.widgetInputPolygon->SetTreeState(_state);

	DumpLog();
}

void SeidelVisualize::DumpTraps(QTextStream& outStream)
{
	if (_state == nullptr)
		return;

	for (auto trap : _state->trapezoids)
	{
		outStream << "Trapezoid " << trap->number << "\n";
		outStream << "inside: " << (trap->inside ? "yes\n" : "no\n");
		outStream << "upper vertex: " << trap->upperPointIndex << "\n";
		outStream << "lower vertex: " << trap->lowerPointIndex << "\n";
		outStream << "left segment: " << trap->leftSegmentIndex << "\n";
		outStream << "right segment: " << trap->rightSegmentIndex << "\n";

		outStream << "upper neighbours: "
			<< (trap->upper1 ? trap->upper1->number : -1) << "  "
			<< (trap->upper2 ? trap->upper2->number : -1) << "  "
			<< (trap->upper3 ? trap->upper3->number : -1)
			<< "\n";

		outStream << "lower neighbours: "
			<< (trap->lower1 ? trap->lower1->number : -1) << "  "
			<< (trap->lower2 ? trap->lower2->number : -1)
			<< "\n\n";
	}
}

void SeidelVisualize::DumpMonChains(QTextStream& outStream)
{
	if (_state == nullptr)
		return;

	for (auto& monChain : _state->monChains)
	{
		for (index_t vertInd : monChain)
			outStream << vertInd << "  ";
		outStream << "\n";
	}
}

void SeidelVisualize::DumpTris(QTextStream& outStream)
{
	if (_state == nullptr)
		return;

	for (index_t i = 0; i < _state->outIndices.size(); ++i)
	{
		outStream << _state->outIndices[i];
		outStream << ((i % 3 == 2) ? "\n" : "  ");
	}
}

void SeidelVisualize::DumpLog()
{
	if (_state == nullptr)
		return;

	{
		QString path = QApplication::applicationDirPath() + "\\trap-dump.txt";
		QFile file { path };
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream outStream { &file };

			DumpTraps(outStream);
		}
	}

	{
		QString path = QApplication::applicationDirPath() + "\\mon-chains-dump.txt";
		QFile file { path };
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream outStream { &file };

			DumpMonChains(outStream);
		}
	}

	{
		QString path = QApplication::applicationDirPath() + "\\tris-dump.txt";
		QFile file { path };
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream outStream { &file };

			DumpTris(outStream);
		}
	}
}

bool SeidelVisualize::LoadPolyFile(const QString& polyFile, std::vector<std::vector<math3d::vec2f>>& outlines)
{
	std::vector<math3d::vec2f> points;
	QFile file { polyFile };
	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream inStream { &file };
		QString line;
		while ((line = inStream.readLine()).isEmpty() == false)
		{
			if (line == "*")
			{
				if (!points.empty())
					outlines.push_back(std::move(points));
			}
			else
			{
				auto strList = line.split(' ');
				if (strList.size() == 2)
				{
					points.push_back({ strList[0].toFloat(), strList[1].toFloat() });
				}
			}
		}

		if (!points.empty())
			outlines.push_back(std::move(points));

		return true;
	}

	return false;
}

bool SeidelVisualize::SavePolyFile(const QString& polyFile, const std::vector<std::vector<math3d::vec2f>>& outlines)
{
	QFile file { polyFile };
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream outStream { &file };
		for (index_t i = 0; i < outlines.size(); ++i)
		{
			if (i > 0)
				outStream << "*\n";

			auto& outl = outlines[i];
			for (auto& pt : outl)
			{
				outStream << pt.x << " " << pt.y << "\n";
			}
		}

		return true;
	}

	return false;
}
