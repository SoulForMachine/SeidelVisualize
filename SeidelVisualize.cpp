#include "SeidelVisualize.h"
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

	connect(ui.actionViewTrapezoids, &QAction::triggered, this, &SeidelVisualize::OnActionViewTrapezoids);
	connect(ui.actionOptionsRandomizeSegments, &QAction::triggered, this, &SeidelVisualize::OnActionOptionsRandSeg);
	connect(actionGroupResults, &QActionGroup::triggered, this, &SeidelVisualize::OnActionViewResult);
	connect(actionGroupTrisWind, &QActionGroup::triggered, this, &SeidelVisualize::OnActionOptionsTrisWinding);
}

SeidelVisualize::~SeidelVisualize()
{
	delete _state;
}

void SeidelVisualize::OnEditFinished()
{
	_dbgSteps = std::numeric_limits<size_t>::max();
	TriangulateAndDisplay();
}

void SeidelVisualize::OnActionLoad()
{
	std::vector<math3d::vec2f> points;

	auto fileName = QFileDialog::getOpenFileName(this, "Load polygon", "", "Polygon files (*.poly)");

	if (!fileName.isEmpty())
	{
		QFile file { fileName };
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream inStream { &file };
			QString line;
			while ((line = inStream.readLine()).isEmpty() == false)
			{
				auto strList = line.split(' ');
				if (strList.size() == 2)
				{
					points.push_back({ strList[0].toFloat(), strList[1].toFloat() });
				}
			}
		}
	}

	if (points.size() >= 3)
	{
		ui.widgetInputPolygon->SetPolygon(points);
		_dbgSteps = std::numeric_limits<size_t>::max();
		TriangulateAndDisplay();
	}
}

void SeidelVisualize::OnActionSave()
{
	if (!ui.widgetInputPolygon->IsEditing())
	{
		const auto& points = ui.widgetInputPolygon->GetPoints();

		auto fileName = QFileDialog::getSaveFileName(this, "Save polygon", "", "Polygon files (*.poly)");

		if (!fileName.isEmpty())
		{
			QFile file { fileName };
			if (file.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				QTextStream outStream { &file };
				for (auto pt : points)
				{
					outStream << pt.x << " " << pt.y << "\n";
				}
			}
		}
	}
}

void SeidelVisualize::OnActionReset()
{
	ui.widgetInputPolygon->ResetAndEdit();
	ui.widgetTrapTree->SetTreeState(nullptr);
	delete _state;
	_state = nullptr;
	_dbgSteps = std::numeric_limits<size_t>::max();
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
		_dbgSteps = std::numeric_limits<size_t>::max();
		TriangulateAndDisplay();
	}
}

void SeidelVisualize::OnActionTrapNextStep()
{
	if (!ui.widgetInputPolygon->IsEditing() && !_randSegments)
	{
		if (_dbgSteps < std::numeric_limits<size_t>::max())
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
	_dbgSteps = std::numeric_limits<size_t>::max();
	TriangulateAndDisplay();
}

void SeidelVisualize::OnActionOptionsTrisWinding(QAction* action)
{
	if (action == ui.actionOptionsTrisWindCCW)
		_triangleWinding = Geometry::Winding::CCW;
	else if (action == ui.actionOptionsTrisWindCW)
		_triangleWinding = Geometry::Winding::CW;

	_dbgSteps = std::numeric_limits<size_t>::max();
	TriangulateAndDisplay();
}

void SeidelVisualize::TriangulateAndDisplay()
{
	if (ui.widgetInputPolygon->IsEditing())
		return;

	const auto& points = ui.widgetInputPolygon->GetPoints();

	delete _state;
	_state = new Geometry::TriangulationState { points.data(), points.size() };
	_state->randomizeSegments = false;
	_state->dbgSteps = _dbgSteps;
	_state->triangleWinding = _triangleWinding;
	_state->randomizeSegments = _randSegments;

	Geometry::TriangulatePolygon_Seidel(*_state);

	_dbgSteps -= _state->dbgSteps;

	ui.widgetTrapTree->SetTreeState(_state);
	ui.widgetInputPolygon->SetTreeState(_state);

	DumpLog();
}

void SeidelVisualize::DumpTree(QTextStream& outStream)
{
	if (_state == nullptr)
		return;

	for (auto trap : _state->trapezoids)
	{
		outStream << "Trapezoid " << trap->number << "\n";
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
		for (int vertInd : monChain)
			outStream << vertInd << "  ";
		outStream << "\n";
	}
}

void SeidelVisualize::DumpTris(QTextStream& outStream)
{
	if (_state == nullptr)
		return;

	for (size_t i = 0; i < _state->outIndices.size(); ++i)
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
		QString path = QApplication::applicationDirPath() + "\\tree-dump.txt";
		QFile file { path };
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream outStream { &file };

			DumpTree(outStream);
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
