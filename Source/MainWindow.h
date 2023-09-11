#ifndef _MAIN_WINDOW_H_
#define _MAIN_WINDOW_H_

#include <nanogui/screen.h>
#include "Common.h"

class MenuPanel;
class StepThroughPanel;
class SplitterWidget;
class PolygonWidget;
class TrapTreeWidget;

class MainWindow : public nanogui::Screen
{
public:
	MainWindow();
	~MainWindow() = default;

	virtual bool resize_event(const nanogui::Vector2i& size) override;
	virtual void perform_layout(NVGcontext* ctx) override;
	virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

private:
	void OnSavePolyFile();
	void OnLoadPolyFile();
	void OnStepThrough(bool enable);
	void OnSaveTriangleIndices();
	void OnSaveTrianglePoints();

	MenuPanel* _menuPanel = nullptr;
	StepThroughPanel* _stepThroughPanel = nullptr;
	SplitterWidget* _splitterWidget = nullptr;
	PolygonWidget* _polygonWidget = nullptr;
	TrapTreeWidget* _trapTreeWidget = nullptr;
	int_t _currentStep = 0;
	int_t _numTrapSteps = 0;
	int_t _totalNumSteps = 0;
};

#endif // _MAIN_WINDOW_H_
