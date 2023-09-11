#include "StepThroughPanel.h"
#include <nanogui/button.h>
#include <nanogui/slider.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <nanogui/icons.h>
#include <GLFW/glfw3.h>
#include <nanovg.h>
#include "IntSliderWidget.h"

constexpr NVGcolor SeparatorColor = { 0.2f, 0.2f, 0.2f, 1.0f };

StepThroughPanel::StepThroughPanel(nanogui::Widget* parent, const Callbacks& callbacks) :
	nanogui::Widget(parent)
{
	set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 10));

	_btnToStart = new nanogui::Button(this, "", FA_FAST_BACKWARD);
	_btnToStart->set_tooltip("Go to stage's first step. (Home)");
	_btnToStart->set_callback([this, gotoFirstStep = callbacks.gotoFirstStep]() {
		int_t curStep = gotoFirstStep();
		_sldSteps->set_value(curStep);
		_lblCurrentStep->set_caption(FormatCaption(curStep, _totalNumSteps));
	});

	_btnPrevStep = new nanogui::Button(this, "", FA_STEP_BACKWARD);
	_btnPrevStep->set_tooltip("Previous step. (Left arrow)");
	_btnPrevStep->set_callback([this, prevStep = callbacks.prevStep]() {
		int_t curStep = prevStep();
		_sldSteps->set_value(curStep);
		_lblCurrentStep->set_caption(FormatCaption(curStep, _totalNumSteps));
	});

	_btnNextStep = new nanogui::Button(this, "", FA_STEP_FORWARD);
	_btnNextStep->set_tooltip("Next step. (Right arrow)");
	_btnNextStep->set_callback([this, nextStep = callbacks.nextStep]() {
		int_t curStep = nextStep();
		_sldSteps->set_value(curStep);
		_lblCurrentStep->set_caption(FormatCaption(curStep, _totalNumSteps));
	});

	_btnToEnd = new nanogui::Button(this, "", FA_FAST_FORWARD);
	_btnToEnd->set_tooltip("Go to stage's last step. (End)");
	_btnToEnd->set_callback([this, gotoLastStep = callbacks.gotoLastStep]() {
		int_t curStep = gotoLastStep();
		_sldSteps->set_value(curStep);
		_lblCurrentStep->set_caption(FormatCaption(curStep, _totalNumSteps));
	});

	_lblCurrentStep = new nanogui::Label(this, "");
	_lblCurrentStep->set_fixed_width(120);

	_sldSteps = new IntSliderWidget(this);
	_sldSteps->set_callback([this, seekTo = callbacks.seekTo](int value) {
		seekTo(value);
		_lblCurrentStep->set_caption(FormatCaption(value, _totalNumSteps));
	});
}

void StepThroughPanel::SetNumSteps(int_t totalNumSteps, int_t numTrapSteps)
{
	_numTrapSteps = numTrapSteps;
	_totalNumSteps = totalNumSteps;
	_lblCurrentStep->set_caption(FormatCaption(0, _totalNumSteps));
	_sldSteps->set_range({ 0, static_cast<int>(_totalNumSteps) });
	_sldSteps->set_value(0);
	_sldSteps->set_marker_value(numTrapSteps);
}

void StepThroughPanel::draw(NVGcontext* ctx)
{
	nanogui::Widget::draw(ctx);

	// Separator line at the bottom.
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, position().x(), position().y() + height() - 1);
	nvgLineTo(ctx, position().x() + width(), position().y() + height() - 1);
	nvgStrokeWidth(ctx, 1.0f);
	nvgStrokeColor(ctx, SeparatorColor);
	nvgStroke(ctx);
}

void StepThroughPanel::perform_layout(NVGcontext* ctx)
{
	nanogui::Widget::perform_layout(ctx);

	int sliderWidth = width() - _sldSteps->position().x() - 10;
	if (sliderWidth > _sldSteps->width())
		_sldSteps->set_width(sliderWidth);
}

bool StepThroughPanel::keyboard_event(int key, int scancode, int action, int modifiers)
{
	if (nanogui::Widget::keyboard_event(key, scancode, action, modifiers))
		return true;

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_HOME:
			_btnToStart->callback()();
			return true;

		case GLFW_KEY_END:
			_btnToEnd->callback()();
			return true;
		}
	}

	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		switch (key)
		{
		case GLFW_KEY_LEFT:
			_btnPrevStep->callback()();
			return true;

		case GLFW_KEY_RIGHT:
			_btnNextStep->callback()();
			return true;
		}
	}

	return false;
}

std::string StepThroughPanel::FormatCaption(int_t curStep, int_t numSteps)
{
	return "Step " + std::to_string(curStep) + "/" + std::to_string(numSteps);
}
