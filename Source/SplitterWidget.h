#ifndef _SPLITTER_WIDGET_H_
#define _SPLITTER_WIDGET_H_

#include <nanogui/widget.h>

// A widget class for sizing two child widgets split by a vertical splitter bar.
class SplitterWidget : public nanogui::Widget
{
public:
	SplitterWidget(nanogui::Widget* parent);

	void SetLeftWidget(nanogui::Widget* widget);
	void SetRightWidget(nanogui::Widget* widget);
	void SetSplitterPosition(float position);

	virtual void perform_layout(NVGcontext* ctx) override;
	virtual void draw(NVGcontext* ctx) override;
	virtual bool mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers) override;
	virtual bool mouse_button_event(const nanogui::Vector2i& p, int button, bool down, int modifiers) override;

private:
	static constexpr int SpliterWidth = 10;
	nanogui::Widget* _leftWidget = nullptr;
	nanogui::Widget* _rightWidget = nullptr;
	float _splitterPos = 0.5f;
};

#endif // _SPLITTER_WIDGET_H_
