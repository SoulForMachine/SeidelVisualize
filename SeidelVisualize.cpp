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
}

SeidelVisualize::~SeidelVisualize()
{
	delete _state;
}

void SeidelVisualize::OnEditFinished()
{
	const auto& points = ui.widgetInputPolygon->GetPoints();
	std::vector<unsigned short> outIndices;

	delete _state;
	_state = new Geometry::TrapezoidTreeState { points.data(), points.size() };

	Geometry::TriangulatePolygon_Seidel(*_state, outIndices);

	ui.widgetTrapTree->SetTreeState(_state);
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

		delete _state;
		_state = new Geometry::TrapezoidTreeState { points.data(), points.size() };

		std::vector<unsigned short> outIndices;
		Geometry::TriangulatePolygon_Seidel(*_state, outIndices);

		ui.widgetTrapTree->SetTreeState(_state);
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
