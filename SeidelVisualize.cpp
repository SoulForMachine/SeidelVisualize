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
	TriangulateAndDisplay(points);
}

void SeidelVisualize::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Up)
	{
		_dbgSteps = std::numeric_limits<size_t>::max();
		const auto& points = ui.widgetInputPolygon->GetPoints();
		TriangulateAndDisplay(points);
	}
	else if (event->key() == Qt::Key_Down)
	{
		_dbgSteps = 0;
		const auto& points = ui.widgetInputPolygon->GetPoints();
		TriangulateAndDisplay(points);
	}
	else if (event->key() == Qt::Key_Left)
	{
		const auto& points = ui.widgetInputPolygon->GetPoints();

		if (_dbgSteps > 0 && points.size() >= 3)
		{
			_dbgSteps -= 1;
			TriangulateAndDisplay(points);
		}
	}
	else if (event->key() == Qt::Key_Right)
	{
		const auto& points = ui.widgetInputPolygon->GetPoints();

		if (points.size() >= 3)
		{
			_dbgSteps += 1;
			TriangulateAndDisplay(points);
		}
	}
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

void SeidelVisualize::TriangulateAndDisplay(const std::vector<math3d::vec2f>& points)
{
	delete _state;
	_state = new Geometry::TrapezoidTreeState { points.data(), points.size() };
	_state->dbgSteps = _dbgSteps;

	std::vector<unsigned short> outIndices;
	Geometry::TriangulatePolygon_Seidel(*_state, outIndices);

	ui.widgetTrapTree->SetTreeState(_state);
	ui.widgetInputPolygon->SetTreeState(_state);

	DumpLog();
}

void DumpTree(QTextStream& outStream, Geometry::TrapezoidationTreeNode* node)
{
	if (node == nullptr)
		return;

	if (node->type == Geometry::TrapezoidationTreeNode::Type::TRAPEZOID)
	{
		auto trap = node->trapezoid;

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
	else
	{
		DumpTree(outStream, node->left);
		DumpTree(outStream, node->right);
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

		DumpTree(outStream, _state->treeRootNode);
	}
}
