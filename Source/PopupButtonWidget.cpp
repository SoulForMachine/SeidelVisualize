#include "PopupButtonWidget.h"

nanogui::Vector2i PopupButtonWidget::preferred_size(NVGcontext* ctx) const
{
	// We want just the button's preferred size, without adding extra space.
	return nanogui::Button::preferred_size(ctx);
}

void PopupButtonWidget::draw(NVGcontext* ctx)
{
	auto* popupWnd = popup();
	// There is a bug in nanogui (in Popup::refresh_relative_placement())
	// which allows the popup to be moved by the user if popup button is not in a window.
	// Reset the popup position here in case it was moved.
	if (popupWnd->parent_window() == nullptr)
	{
		nanogui::Vector2i offset(
			width() + popupWnd->anchor_size() + 1,
			height() / 2 - popupWnd->anchor_offset());

		popupWnd->set_position(absolute_position() + offset);
	}

	nanogui::PopupButton::draw(ctx);
}
