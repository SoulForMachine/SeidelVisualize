#include "MainWindow.h"
#include <filesystem>
#include <nanogui/layout.h>
#include <nanogui/slider.h>
#include <nanogui/button.h>
#include <nanogui/messagedialog.h>
#include <nanogui/common.h>
#include "SplitterWidget.h"
#include "PolygonWidget.h"
#include "TrapTreeWidget.h"
#include "MenuPanel.h"
#include "StepThroughPanel.h"
#include "SeidelTriangulator.h"
#include "Serialization.h"

MainWindow::MainWindow() :
	nanogui::Screen(nanogui::Vector2i(1024, 768), "Seidel Triangulation Visualization")
{
	// Menu panel.

	MenuPanel::Callbacks menuCallbacks;
	menuCallbacks.polySave = [this]() { OnSavePolyFile(); };
	menuCallbacks.polyLoad = [this]() { OnLoadPolyFile(); };
	menuCallbacks.polyClear = [this]() { _polygonWidget->ClearAllOutlines(); _trapTreeWidget->ResetView(); };
	menuCallbacks.viewStepByStep = [this](bool enable) { OnStepThrough(enable); };
	menuCallbacks.viewAbout = [this]() {
		new nanogui::MessageDialog(
			this, nanogui::MessageDialog::Type::Information, "About",
			"Seidel Triangulation Visualization\nMilan Davidovic\nSeptember 2021");
	};
	menuCallbacks.trapFillRule = [this](int fillRule) { _polygonWidget->SetFillRule(static_cast<SeidelTriangulator::FillRule>(fillRule)); };
	menuCallbacks.trapRandomizeSegments = [this](bool randomize) { _polygonWidget->SetRandomizeSegments(randomize); };
	menuCallbacks.trianWinding = [this](int winding) { _polygonWidget->SetTriangleWinding(static_cast<SeidelTriangulator::Winding>(winding)); };
	menuCallbacks.trianSaveIndices = [this]() { OnSaveTriangleIndices(); };
	menuCallbacks.trianSavePoints = [this]() { OnSaveTrianglePoints(); };

	_menuPanel = new MenuPanel(this, menuCallbacks);

	// Step by step panel, initially hidden.

	StepThroughPanel::Callbacks stepCallbacks;
	stepCallbacks.gotoFirstStep = [this]() -> int_t {
		_currentStep = (_currentStep > _numTrapSteps) ? _numTrapSteps : 0;
		_polygonWidget->SetCurrentStep(_currentStep, _numTrapSteps);
		return _currentStep;
	};
	stepCallbacks.gotoLastStep = [this]() -> int_t {
		_currentStep = (_currentStep < _numTrapSteps) ? _numTrapSteps : _totalNumSteps;
		_polygonWidget->SetCurrentStep(_currentStep, _numTrapSteps);
		return _currentStep;
	};
	stepCallbacks.nextStep = [this]() -> int_t {
		if (_currentStep < _totalNumSteps)
			_polygonWidget->SetCurrentStep(++_currentStep, _numTrapSteps);
		return _currentStep;
	};
	stepCallbacks.prevStep = [this]() -> int_t {
		if (_currentStep > 0)
			_polygonWidget->SetCurrentStep(--_currentStep, _numTrapSteps);
		return _currentStep;
	};
	stepCallbacks.seekTo = [this](int_t step) {
		_currentStep = step;
		_polygonWidget->SetCurrentStep(step, _numTrapSteps);
	};

	_stepThroughPanel = new StepThroughPanel(this, stepCallbacks);
	_stepThroughPanel->set_visible(false);

	// Splitter widget with two child widgets: polygon and trapezoidation tree.

	_splitterWidget = new SplitterWidget(this);

	PolygonWidget::Callbacks polyCallbacks;
	polyCallbacks.triangulatorUpdated = [this]() { _trapTreeWidget->SetTriangulator(_polygonWidget->GetTriangulator()); };
	polyCallbacks.polygonAvailable = [this](bool available, bool valid) { _menuPanel->SetPolygonAvailable(available, valid); };
	_polygonWidget = new PolygonWidget(_splitterWidget, polyCallbacks);

	TrapTreeWidget::Callbacks treeCallbacks;
	treeCallbacks.hoveredNodeChanged = [this](const SeidelTriangulator::TreeNode* trapNode) { _polygonWidget->SetTrapNodeHighlight(trapNode); };
	treeCallbacks.selectedNodeChanged = [this](const SeidelTriangulator::TreeNode* trapNode) { _polygonWidget->SetTrapNodeSelection(trapNode); };
	treeCallbacks.nodeDoubleClicked = [this](const SeidelTriangulator::TreeNode* trapNode) { _polygonWidget->CenterTrapNode(trapNode); };
	_trapTreeWidget = new TrapTreeWidget(_splitterWidget, treeCallbacks);

	_splitterWidget->SetLeftWidget(_polygonWidget);
	_splitterWidget->SetRightWidget(_trapTreeWidget);

	nanogui::Screen::perform_layout();
}

bool MainWindow::resize_event(const nanogui::Vector2i& size)
{
	nanogui::Screen::perform_layout();
	return nanogui::Screen::resize_event(size);
}

void MainWindow::perform_layout(NVGcontext* ctx)
{
	_menuPanel->set_position({ 0, 0 });
	_menuPanel->set_fixed_width(width());

	auto splitterOffset = _menuPanel->height() + (_stepThroughPanel->visible() ? _stepThroughPanel->height() : 0);
	_splitterWidget->set_position({ 0, splitterOffset });
	_splitterWidget->set_size({ width(), height() - splitterOffset });

	if (_stepThroughPanel->visible())
	{
		_stepThroughPanel->set_position({ 0, _menuPanel->height() });
		_stepThroughPanel->set_fixed_width(width());
	}

	nanogui::Screen::perform_layout(ctx);
}

bool MainWindow::keyboard_event(int key, int scancode, int action, int modifiers)
{
	if (nanogui::Screen::keyboard_event(key, scancode, action, modifiers))
		return true;

	// If the default implementation does not handle the keybord event, give the menu and step-by-step panels
	// a chance to handle it even though they are not in focus.

	if (_menuPanel->keyboard_event(key, scancode, action, modifiers))
		return true;

	if (_stepThroughPanel->visible() && _stepThroughPanel->keyboard_event(key, scancode, action, modifiers))
		return true;

	return false;
}

void MainWindow::OnSavePolyFile()
{
	const auto& outlines = _polygonWidget->GetPolygonOutlines();

	if (!outlines.empty())
	{
		auto filePathStr = nanogui::file_dialog({ { "poly", "Polygon File" } }, true);

		if (!filePathStr.empty())
		{
			std::filesystem::path filePath(filePathStr);
			if (!filePath.has_extension() && !std::filesystem::exists(filePath))
				filePath.replace_extension("poly");

			if (!SavePolyFile(filePath.string(), outlines))
			{
				new nanogui::MessageDialog(this, nanogui::MessageDialog::Type::Warning, "Error", "Failed to save file: " + filePath.string());
			}
		}
	}
}

void MainWindow::OnLoadPolyFile()
{
	auto filePath = nanogui::file_dialog({ { "poly", "Polygon File" } }, false);

	if (!filePath.empty())
	{
		OutlineList outlines;
		if (LoadPolyFile(filePath, outlines))
		{
			if (!outlines.empty())
			{
				_polygonWidget->SetPolygonOutlines(std::move(outlines));
				_trapTreeWidget->ResetView();
			}
		}
		else
		{
			new nanogui::MessageDialog(this, nanogui::MessageDialog::Type::Warning, "Error", "Failed to load file: " + filePath);
		}
	}
}

void MainWindow::OnStepThrough(bool enable)
{
	auto triangulator = _polygonWidget->GetTriangulator();
	if (triangulator == nullptr)
		return;

	if (enable)
	{
		_numTrapSteps = _polygonWidget->GetNumTrapezoidationSteps();
		_totalNumSteps = _numTrapSteps + _polygonWidget->GetNumTriangulationSteps();
		if (_totalNumSteps > 0)
		{
			_stepThroughPanel->SetNumSteps(_totalNumSteps, _numTrapSteps);
			_stepThroughPanel->set_visible(true);
			_currentStep = 0;
			_polygonWidget->EnableStepByStepTrapezoidation(true);
		}
	}
	else
	{
		if (_polygonWidget->GetState() == PolygonWidget::State::StepByStepTrapezoidation)
		{
			_stepThroughPanel->set_visible(false);
			_polygonWidget->EnableStepByStepTrapezoidation(false);
		}
	}

	nanogui::Screen::perform_layout();
}

void MainWindow::OnSaveTriangleIndices()
{
	const auto& indices = _polygonWidget->GetTriangleIndices();

	if (!indices.empty())
	{
		auto filePathStr = nanogui::file_dialog({ { "tind", "Triangle Indices File" } }, true);

		if (!filePathStr.empty())
		{
			std::filesystem::path filePath(filePathStr);
			if (!filePath.has_extension() && !std::filesystem::exists(filePath))
				filePath.replace_extension("tind");

			if (!SaveTriangleIndices(filePath.string(), indices))
			{
				new nanogui::MessageDialog(this, nanogui::MessageDialog::Type::Warning, "Error", "Failed to save file: " + filePath.string());
			}
		}
	}
}

void MainWindow::OnSaveTrianglePoints()
{
	auto triangulator = _polygonWidget->GetTriangulator();
	if (triangulator == nullptr)
		return;

	const auto& points = triangulator->GetPointCoords();
	const auto& indices = _polygonWidget->GetTriangleIndices();

	if (!points.empty() && !indices.empty())
	{
		auto filePathStr = nanogui::file_dialog({ { "tpts", "Triangle Points File" } }, true);

		if (!filePathStr.empty())
		{
			std::filesystem::path filePath(filePathStr);
			if (!filePath.has_extension() && !std::filesystem::exists(filePath))
				filePath.replace_extension("tpts");

			if (!SaveTrianglePoints(filePath.string(), indices, points))
			{
				new nanogui::MessageDialog(this, nanogui::MessageDialog::Type::Warning, "Error", "Failed to save file: " + filePath.string());
			}
		}
	}
}
