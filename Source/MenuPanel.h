#ifndef _MENU_PANEL_H_
#define _MENU_PANEL_H_

#include <functional>
#include <nanogui/widget.h>

namespace nanogui
{
class Button;
}

class MenuPanel : public nanogui::Widget
{
public:
	struct Callbacks
	{
		std::function<void()> polySave;
		std::function<void()> polyLoad;
		std::function<void()> polyClear;

		std::function<void(bool)> viewStepByStep;
		std::function<void()> viewAbout;

		std::function<void(int)> trapFillRule;
		std::function<void(bool)> trapRandomizeSegments;

		std::function<void(int)> trianWinding;
		std::function<void()> trianSaveIndices;
		std::function<void()> trianSavePoints;
	};

	MenuPanel(nanogui::Widget* parent, const Callbacks& callbacks);
	
	void SetPolygonAvailable(bool available, bool valid);

	virtual void draw(NVGcontext* ctx) override;
	virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

private:
	void OnStepByStepModeEnabled(bool enabled);

	std::vector<nanogui::Widget*> _disabledInStepByStepMode;
	std::vector<nanogui::Widget*> _disabledWhenNoPolygon;
	std::vector<nanogui::Widget*> _disabledWhenInvalidPolygon;
	nanogui::Button* _btnLoad = nullptr;
	nanogui::Button* _btnSave = nullptr;
	nanogui::Button* _btnStepByStep = nullptr;
};


#endif // _MENU_PANEL_H_
