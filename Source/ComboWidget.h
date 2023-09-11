#ifndef _COMBO_WIDGET_H_
#define _COMBO_WIDGET_H_

#include <nanogui/combobox.h>

// A customized combo box widget class with it's own preferred size calculation.
// It also fixes a bug in nanogui::ComboBox which allows the popup to be moved
// by the user if combo box is not placed in a nanogui::Window.
class ComboWidget : public nanogui::ComboBox
{
public:
	ComboWidget(Widget* parent, const std::vector<std::string>& items) :
		nanogui::ComboBox(parent, items)
	{ }

	virtual nanogui::Vector2i preferred_size(NVGcontext* ctx) const override;
	virtual void draw(NVGcontext* ctx) override;
};

#endif // _COMBO_WIDGET_H_
