#ifndef _POPUP_BUTTON_WIDGET_H_
#define _POPUP_BUTTON_WIDGET_H_

#include <nanogui/popupbutton.h>

// A customized popup button widget class with it's own preferred size calculation.
// It also fixes a bug in nanogui::PopupButton which allows the popup to be moved
// by the user if popup button is not placed in a nanogui::Window.
class PopupButtonWidget : public nanogui::PopupButton
{
public:
	PopupButtonWidget(Widget* parent, const std::string& caption = "Untitled", int buttonIcon = 0) :
		nanogui::PopupButton(parent, caption, buttonIcon)
	{ }

	virtual nanogui::Vector2i preferred_size(NVGcontext* ctx) const override;
	virtual void draw(NVGcontext* ctx) override;
};

#endif // _POPUP_BUTTON_WIDGET_H_
