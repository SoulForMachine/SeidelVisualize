#include "MenuPanel.h"
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/slider.h>
#include <nanogui/icons.h>
#include <nanovg.h>
#include <GLFW/glfw3.h>
#include "ComboWidget.h"

constexpr NVGcolor SeparatorColor = { 0.2f, 0.2f, 0.2f, 1.0f };
const nanogui::Color SectionLabelColor = { 0.541f, 0.658f, 1.0f, 1.0f };
constexpr float IconExtraScale = 1.4f;

MenuPanel::MenuPanel(nanogui::Widget* parent, const Callbacks& callbacks) :
	nanogui::Widget(parent)
{
	set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Minimum, 10, 35));
	
	// Loading, saving, clearing polygons.
	{
		auto fileWidget = new nanogui::Widget(this);
		fileWidget->set_layout(new nanogui::GroupLayout(2, 4));

		auto lblFile = new nanogui::Label(fileWidget, "Polygons", "sans-bold", 20);
		lblFile->set_color(SectionLabelColor);

		_btnLoad = new nanogui::Button(fileWidget, "Load");
		_btnLoad->set_callback(callbacks.polyLoad);
		_btnLoad->set_icon(FA_DOWNLOAD);
		_btnLoad->set_fixed_width(100);
		_btnLoad->set_icon_extra_scale(IconExtraScale);
		_btnLoad->set_tooltip("Load polygons from a file. (Ctrl+L)");

		_btnSave = new nanogui::Button(fileWidget, "Save");
		_btnSave->set_callback(callbacks.polySave);
		_btnSave->set_icon(FA_SAVE);
		_btnSave->set_fixed_width(100);
		_btnSave->set_icon_extra_scale(IconExtraScale);
		_btnSave->set_tooltip("Save polygons to a file. (Ctrl+S)");

		auto btnClear = new nanogui::Button(fileWidget, "Clear");
		btnClear->set_callback(callbacks.polyClear);
		btnClear->set_icon(FA_ERASER);
		btnClear->set_fixed_width(100);
		btnClear->set_icon_extra_scale(IconExtraScale);
		btnClear->set_tooltip("Delete all polygons.");

		_disabledInStepByStepMode.push_back(_btnLoad);
		_disabledInStepByStepMode.push_back(_btnSave);
		_disabledInStepByStepMode.push_back(btnClear);

		_disabledWhenNoPolygon.push_back(_btnSave);
		_disabledWhenNoPolygon.push_back(btnClear);

		_disabledWhenInvalidPolygon.push_back(_btnSave);
	}

	// View step-by-step mode, etc.
	{
		auto viewWidget = new nanogui::Widget(this);
		viewWidget->set_layout(new nanogui::GroupLayout(2, 4));

		auto lblView = new nanogui::Label(viewWidget, "View", "sans-bold", 20);
		lblView->set_color(SectionLabelColor);

		_btnStepByStep = new nanogui::Button(viewWidget, "Step by step", FA_WALKING);
		_btnStepByStep->set_flags(nanogui::Button::ToggleButton);
		_btnStepByStep->set_change_callback([this, viewStepByStep = callbacks.viewStepByStep](bool pushed) {
			OnStepByStepModeEnabled(pushed);
			viewStepByStep(pushed);
		});
		_btnStepByStep->set_icon_extra_scale(IconExtraScale);
		_btnStepByStep->set_tooltip("Go through trapezoidation and triangulation step by step. (F5)");

		auto btnAbout = new nanogui::Button(viewWidget, "About", FA_GRADUATION_CAP);
		btnAbout->set_callback(callbacks.viewAbout);
		btnAbout->set_icon_extra_scale(IconExtraScale);
		btnAbout->set_tooltip("Info about the application.");

		_disabledWhenNoPolygon.push_back(_btnStepByStep);

		_disabledWhenInvalidPolygon.push_back(_btnStepByStep);
	}

	// Trapezoidation options.
	{
		auto trapWidget = new nanogui::Widget(this);
		trapWidget->set_layout(new nanogui::GroupLayout(2, 4));

		auto lblTrap= new nanogui::Label(trapWidget, "Trapezoidation", "sans-bold", 20);
		lblTrap->set_color(SectionLabelColor);

		auto trapFillRuleWidget = new nanogui::Widget(trapWidget);
		trapFillRuleWidget->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Fill));

		auto lblFillRule = new nanogui::Label(trapFillRuleWidget, "Fill rule: ");
		auto cmbFillRule = new ComboWidget(trapFillRuleWidget, { "Non-zero", "Even-odd" });
		cmbFillRule->set_callback(callbacks.trapFillRule);
		cmbFillRule->set_selected_index(1);

		auto chkRandom = new nanogui::CheckBox(trapWidget, "Randomize segments", callbacks.trapRandomizeSegments);
		chkRandom->set_callback(callbacks.trapRandomizeSegments);
		chkRandom->set_checked(true);

		_disabledInStepByStepMode.push_back(lblFillRule);
		_disabledInStepByStepMode.push_back(cmbFillRule);
		_disabledInStepByStepMode.push_back(chkRandom);
	}

	// Triangulation options.
	{
		auto triangWidget = new nanogui::Widget(this);
		triangWidget->set_layout(new nanogui::GroupLayout(2, 4));

		auto lblTriang = new nanogui::Label(triangWidget, "Triangulation", "sans-bold", 20);
		lblTriang->set_color(SectionLabelColor);

		auto triWindWidget = new nanogui::Widget(triangWidget);
		triWindWidget->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Fill));

		auto lblWinding = new nanogui::Label(triWindWidget, "Winding: ");
		auto cmbWinding = new ComboWidget(triWindWidget, { "Clockwise", "Counter-clockwise" });
		cmbWinding->set_callback(callbacks.trianWinding);
		cmbWinding->set_selected_index(1);

		auto btnExportIndices = new nanogui::Button(triangWidget, "Export indices", FA_LIST_OL);
		btnExportIndices->set_icon_extra_scale(IconExtraScale);
		btnExportIndices->set_callback(callbacks.trianSaveIndices);
		auto btnExportPoints = new nanogui::Button(triangWidget, "Export points", FA_LIST_UL);
		btnExportPoints->set_icon_extra_scale(IconExtraScale);
		btnExportPoints->set_callback(callbacks.trianSavePoints);

		_disabledInStepByStepMode.push_back(lblWinding);
		_disabledInStepByStepMode.push_back(cmbWinding);
		_disabledInStepByStepMode.push_back(btnExportIndices);
		_disabledInStepByStepMode.push_back(btnExportPoints);

		_disabledWhenNoPolygon.push_back(btnExportIndices);
		_disabledWhenNoPolygon.push_back(btnExportPoints);

		_disabledWhenInvalidPolygon.push_back(btnExportIndices);
		_disabledWhenInvalidPolygon.push_back(btnExportPoints);
	}

	SetPolygonAvailable(false, false);
}

void MenuPanel::SetPolygonAvailable(bool available, bool valid)
{
	if (available)
	{
		for (auto widget : _disabledWhenNoPolygon)
			widget->set_enabled(true);

		if (!valid)
		{
			for (auto widget : _disabledWhenInvalidPolygon)
				widget->set_enabled(false);
		}
	}
	else
	{
		for (auto widget : _disabledWhenNoPolygon)
			widget->set_enabled(false);
	}
}

void MenuPanel::draw(NVGcontext* ctx)
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

bool MenuPanel::keyboard_event(int key, int scancode, int action, int modifiers)
{
	if (nanogui::Widget::keyboard_event(key, scancode, action, modifiers))
		return true;

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_S:
			if ((modifiers & GLFW_MOD_CONTROL) != 0 && _btnSave->enabled())
			{
				_btnSave->callback()();
				return true;
			}
			break;

		case GLFW_KEY_L:
			if ((modifiers & GLFW_MOD_CONTROL) != 0 && _btnLoad->enabled())
			{
				_btnLoad->callback()();
				return true;
			}
			break;

		case GLFW_KEY_F5:
			if (_btnStepByStep->enabled())
			{
				_btnStepByStep->set_pushed(!_btnStepByStep->pushed());
				_btnStepByStep->change_callback()(_btnStepByStep->pushed());
				return true;
			}
			break;
		}
	}

	return false;
}

void MenuPanel::OnStepByStepModeEnabled(bool enabled)
{
	for (auto widget : _disabledInStepByStepMode)
	{
		widget->set_enabled(!enabled);
	}
}
