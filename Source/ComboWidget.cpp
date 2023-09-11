#include "ComboWidget.h"

nanogui::Vector2i ComboWidget::preferred_size(NVGcontext* ctx) const
{
	// Set the width of the combo according to the widest item.
	// This way it won't change when the selected item changes.
	int maxWidth = 0;
	for (auto child : popup()->children())
		if (child->width() > maxWidth)
			maxWidth = child->width();

	auto prefSize = Button::preferred_size(ctx);
	return nanogui::Vector2i((maxWidth ? maxWidth : prefSize.x()) + 25, prefSize.y());
}

void ComboWidget::draw(NVGcontext* ctx)
{
	auto* popupWnd = popup();
	// There is a bug in nanogui (in Popup::refresh_relative_placement())
	// which allows the popup to be moved by the user if combo is not in a window.
	// Reset the popup position here in case it was moved.
	if (popupWnd->parent_window() == nullptr)
	{
		nanogui::Vector2i offset(
			width() + popupWnd->anchor_size() + 1,
			height() / 2 - popupWnd->anchor_offset());

		popupWnd->set_position(absolute_position() + offset);
	}

	nanogui::ComboBox::draw(ctx);
}
