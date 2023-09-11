#ifndef _STEP_THROUGH_PANEL_H_
#define _STEP_THROUGH_PANEL_H_

#include <functional>
#include <nanogui/widget.h>
#include "Common.h"

namespace nanogui
{
class Label;
class Button;
}

class IntSliderWidget;

class StepThroughPanel : public nanogui::Widget
{
public:
	struct Callbacks
	{
		std::function<int_t()> gotoFirstStep;
		std::function<int_t()> gotoLastStep;
		std::function<int_t()> nextStep;
		std::function<int_t()> prevStep;
		std::function<void(int_t)> seekTo;
	};

	StepThroughPanel(nanogui::Widget* parent, const Callbacks& callbacks);

	void SetNumSteps(int_t totalNumSteps, int_t numTrapSteps);

	virtual void draw(NVGcontext* ctx) override;
	virtual void perform_layout(NVGcontext* ctx) override;
	virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

private:
	std::string FormatCaption(int_t curStep, int_t numSteps);

	IntSliderWidget* _sldSteps = nullptr;
	nanogui::Label* _lblCurrentStep = nullptr;
	nanogui::Button* _btnToStart = nullptr;
	nanogui::Button* _btnToEnd = nullptr;
	nanogui::Button* _btnPrevStep = nullptr;
	nanogui::Button* _btnNextStep = nullptr;
	int_t _totalNumSteps = 0;
	int_t _numTrapSteps = 0;
};

#endif // _STEP_THROUGH_PANEL_H_
