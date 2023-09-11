#include "SplitterWidget.h"
#include <algorithm>
#include <chrono>
#include <nanovg.h>
#include <GLFW/glfw3.h>
#include <nanogui/screen.h>

SplitterWidget::SplitterWidget(nanogui::Widget* parent) :
	nanogui::Widget(parent)
{
	set_cursor(nanogui::Cursor::HResize);
}

void SplitterWidget::SetLeftWidget(nanogui::Widget* widget)
{
	if (widget == _leftWidget)
		return;

	if (_leftWidget != nullptr)
		remove_child(_leftWidget);

	_leftWidget = widget;
	if (widget != nullptr)
	{
		auto it = std::find(m_children.begin(), m_children.end(), widget);
		if (it == m_children.end())
			nanogui::Widget::add_child(widget);
	}
}

void SplitterWidget::SetRightWidget(nanogui::Widget* widget)
{
	if (widget == _rightWidget)
		return;

	if (_rightWidget != nullptr)
		remove_child(_rightWidget);

	_rightWidget = widget;
	if (widget != nullptr)
	{
		auto it = std::find(m_children.begin(), m_children.end(), widget);
		if (it == m_children.end())
			nanogui::Widget::add_child(widget);
	}
}

void SplitterWidget::SetSplitterPosition(float position)
{
	_splitterPos = std::clamp(position, 0.0f, 1.0f);
	screen()->perform_layout();
}

void SplitterWidget::perform_layout(NVGcontext* ctx)
{
	auto mySize = size();
	int leftWidgetWidth = 0;

	if (mySize.x() <= SpliterWidth)
		return;

	if (_leftWidget != nullptr)
	{
		leftWidgetWidth = static_cast<int>((mySize.x() - SpliterWidth) * _splitterPos);
		_leftWidget->set_position(nanogui::Vector2i(0, 0));
		_leftWidget->set_width(leftWidgetWidth);
		_leftWidget->set_height(mySize.y());
		_leftWidget->perform_layout(ctx);
	}

	if (_rightWidget != nullptr)
	{
		int rightWidgetWidth = mySize.x() - SpliterWidth - leftWidgetWidth;
		_rightWidget->set_position(nanogui::Vector2i(leftWidgetWidth + SpliterWidth, 0));
		_rightWidget->set_width(rightWidgetWidth);
		_rightWidget->set_height(mySize.y());
		_rightWidget->perform_layout(ctx);
	}
}

void SplitterWidget::draw(NVGcontext* ctx)
{
	nanogui::Widget::draw(ctx);
	
	// Fill the splitter widget with single color.
	if (_leftWidget != nullptr && _rightWidget != nullptr)
	{
		nvgBeginPath(ctx);
		int leftWidgetWidth = static_cast<int>((width() - SpliterWidth) * _splitterPos);
		nvgRect(ctx, position().x() + leftWidgetWidth, position().y(), SpliterWidth, height());
		nvgFillColor(ctx, { 0.2f, 0.2f, 0.2f, 1.0f });
		nvgFill(ctx);
	}
}

bool SplitterWidget::mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers)
{
	if (_leftWidget != nullptr && _rightWidget != nullptr)
	{
		if ((button & (1 << GLFW_MOUSE_BUTTON_LEFT)) != 0)
		{
			SetSplitterPosition((width() * _splitterPos + rel.x()) / width());
			return true;
		}
	}

	return false;
}

bool SplitterWidget::mouse_button_event(const nanogui::Vector2i& p, int button, bool down, int modifiers)
{
	if (nanogui::Widget::mouse_button_event(p, button, down, modifiers))
		return true;

	// On left button double click, reset the spliter position to the middle of the parent.
	if (_leftWidget != nullptr && _rightWidget != nullptr)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && !down)
		{
			static auto before = std::chrono::system_clock::now();
			auto now = std::chrono::system_clock::now();
			double diffMs = std::chrono::duration<double, std::milli>(now - before).count();
			before = now;
			if (diffMs > 10 && diffMs < 250)
			{
				SetSplitterPosition(0.5f);
				return true;
			}
		}
	}

	return false;
}
