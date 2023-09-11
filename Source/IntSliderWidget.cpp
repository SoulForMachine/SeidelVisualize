#include "IntSliderWidget.h"
#include <nanovg.h>

IntSliderWidget::IntSliderWidget(nanogui::Widget* parent) :
	nanogui::Slider(parent)
{
}

bool IntSliderWidget::mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers)
{
	if (!m_enabled)
		return false;

	const float kr = (int)(m_size.y() * 0.4f), kshadow = 3;
	const float start_x = kr + kshadow + m_pos.x() - 1;
	const float width_x = m_size.x() - 2 * (kr + kshadow);

	int old_value = static_cast<int>(m_value);
	float value = (p.x() - start_x) / width_x;
	value = value * (m_range.second - m_range.first) + m_range.first;
	m_value = std::min(std::max(value, m_range.first), m_range.second);
	if (m_callback && static_cast<int>(m_value) != old_value)
		m_callback(m_value);

	return true;
}

bool IntSliderWidget::mouse_button_event(const nanogui::Vector2i& p, int button, bool down, int modifiers)
{
	if (!mouse_drag_event(p, { 0, 0 }, button, modifiers))
		return false;

	if (!down)
	{
		m_value = static_cast<int>(m_value);
	}

	return true;
}

void IntSliderWidget::draw(NVGcontext* ctx)
{
	if (_markerValue >= m_range.first && _markerValue <= m_range.second)
	{
		// Used the code for calculating knob position from nanogui::Slider::draw to calculate marker position.
		nanogui::Vector2f center = nanogui::Vector2f(m_pos) + nanogui::Vector2f(m_size) * 0.5f;
		float kr = (int)(m_size.y() * 0.4f), kshadow = 3;
		float start_x = kr + kshadow + m_pos.x();
		float width_x = m_size.x() - 2 * (kr + kshadow);

		nanogui::Vector2f markerPos(start_x + (_markerValue - m_range.first) /
			(m_range.second - m_range.first) * width_x,
			center.y() + 0.5f);

		nvgBeginPath(ctx);
		nvgMoveTo(ctx, markerPos.x(), markerPos.y());
		nvgLineTo(ctx, markerPos.x() - 7.0f, markerPos.y() - 10.0f);
		nvgLineTo(ctx, markerPos.x() + 7.0f, markerPos.y() - 10.0f);
		nvgFillColor(ctx, { 0.9f, 0.0f, 0.0f, 1.0f });
		nvgFill(ctx);
	}

	nanogui::Slider::draw(ctx);
}
