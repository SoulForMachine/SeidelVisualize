#include "SeidelVisualize.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	SeidelVisualize w;
	w.show();
	return a.exec();
}
