#include "SeidelVisualize.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include "TriangulatePolygon_Seidel.h"


SeidelVisualize::SeidelVisualize(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	showMaximized();

	ui.widgetInputPolygon->SetListener(this);

	connect(ui.actionLoad, &QAction::triggered, this, &SeidelVisualize::OnActionLoad);
	connect(ui.actionSave, &QAction::triggered, this, &SeidelVisualize::OnActionSave);
	connect(ui.actionReset, &QAction::triggered, this, &SeidelVisualize::OnActionReset);
	connect(ui.actionResetViews, &QAction::triggered, this, &SeidelVisualize::OnActionResetView);

	connect(ui.actionTrapToStart, &QAction::triggered, this, &SeidelVisualize::OnActionTrapToStart);
	connect(ui.actionTrapToEnd, &QAction::triggered, this, &SeidelVisualize::OnActionTrapToEnd);
	connect(ui.actionTrapNextStep, &QAction::triggered, this, &SeidelVisualize::OnActionTrapNextStep);
	connect(ui.actionTrapPreviousStep, &QAction::triggered, this, &SeidelVisualize::OnActionTrapPrevStep);
}

SeidelVisualize::~SeidelVisualize()
{
	delete _state;
}

void SeidelVisualize::OnEditFinished()
{
	const auto& points = ui.widgetInputPolygon->GetPoints();
	TriangulateAndDisplay(points);
}

void SeidelVisualize::OnActionLoad()
{
	std::vector<math3d::vec2f> points;

	auto fileName = QFileDialog::getOpenFileName(this, "Load polygon", QApplication::applicationDirPath(), "Polygon files (*.poly)");

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
		TriangulateAndDisplay(points);
	}
}

void SeidelVisualize::OnActionSave()
{
	if (!ui.widgetInputPolygon->IsEditing())
	{
		const auto& points = ui.widgetInputPolygon->GetPoints();

		auto fileName = QFileDialog::getSaveFileName(this, "Save polygon", QApplication::applicationDirPath(), "Polygon files (*.poly)");

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
}

void SeidelVisualize::OnActionResetView()
{
	ui.widgetInputPolygon->ResetView();
	ui.widgetTrapTree->ResetView();
}

void SeidelVisualize::OnActionTrapToStart()
{
	_dbgSteps = 0;
	const auto& points = ui.widgetInputPolygon->GetPoints();

	if (points.size() >= 3)
	{
		TriangulateAndDisplay(points);
	}
}

void SeidelVisualize::OnActionTrapToEnd()
{
	_dbgSteps = std::numeric_limits<size_t>::max();
	const auto& points = ui.widgetInputPolygon->GetPoints();

	if (points.size() >= 3)
	{
		TriangulateAndDisplay(points);
	}
}

void SeidelVisualize::OnActionTrapNextStep()
{
	_dbgSteps += 1;
	const auto& points = ui.widgetInputPolygon->GetPoints();

	if (points.size() >= 3)
	{
		TriangulateAndDisplay(points);
	}
}

void SeidelVisualize::OnActionTrapPrevStep()
{
	if (_dbgSteps > 0)
		_dbgSteps -= 1;

	const auto& points = ui.widgetInputPolygon->GetPoints();

	if (points.size() >= 3)
	{
		TriangulateAndDisplay(points);
	}
}

void SeidelVisualize::TriangulateAndDisplay(const std::vector<math3d::vec2f>& points)
{
	delete _state;
	_state = new Geometry::TriangulationState { points.data(), points.size() };
	_state->dbgSteps = _dbgSteps;

	Geometry::TriangulatePolygon_Seidel(*_state);

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
		outStream << "upper vertex: " << trap->upperVertexIndex << "\n";
		outStream << "lower vertex: " << trap->lowerVertexIndex << "\n";
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

void SeidelVisualize::DumpLog()
{
	if (_state == nullptr)
		return;

	QString path = QApplication::applicationDirPath() + "\\tree-dump.txt";
	QFile file { path };
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream outStream { &file };

		DumpTree(outStream);
	}
}
