#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_SeidelVisualize.h"

class SeidelVisualize : public QMainWindow
{
	Q_OBJECT

public:
	SeidelVisualize(QWidget *parent = Q_NULLPTR);

private:
	Ui::SeidelVisualizeClass ui;
};
