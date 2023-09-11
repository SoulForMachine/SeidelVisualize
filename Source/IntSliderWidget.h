#ifndef _INT_SLIDER_WIDGET_H_
#define _INT_SLIDER_WIDGET_H_

#include <nanogui/slider.h>

// An adapter class for slider widget which uses integers instead of floats for its value,
// as nanogui::Slider does. This prevents the callback function to be called for non integral
// value changes, when that isn't necessary or wanted.
class IntSliderWidget : protected nanogui::Slider
{
public:
	using nanogui::Widget::width;
	using nanogui::Widget::set_width;
	using nanogui::Widget::position;

	IntSliderWidget(nanogui::Widget* parent);

	int value() const { return static_cast<int>(m_value); }
	void set_value(int value) { m_value = static_cast<float>(value); }

	std::pair<int, int> range() const { return std::make_pair(static_cast<int>(m_range.first), static_cast<int>(m_range.second)); }
	void set_range(std::pair<int, int> range) { m_range = std::make_pair(static_cast<float>(range.first), static_cast<float>(range.second)); }

	std::function<void(int)> callback() const { return [this](int value) { m_callback(static_cast<float>(value)); }; }
	void set_callback(const std::function<void(int)>& callback) { m_callback = [callback](float value) { callback(static_cast<int>(value)); }; }

	int marker_value() const { return _markerValue; }
	void set_marker_value(int value) { _markerValue = value; }

	virtual bool mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers) override;
	virtual bool mouse_button_event(const nanogui::Vector2i& p, int button, bool down, int modifiers) override;
	
	virtual void draw(NVGcontext* ctx) override;

private:
	int _markerValue = -1;
};

#endif // _INT_SLIDER_WIDGET_H_
