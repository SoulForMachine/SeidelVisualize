#include "SeidelVisualize.h"
#include <QtWidgets/QApplication>
#include <QCommandLineParser>
#include <QTextStream>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QCommandLineParser argParser;
	argParser.setApplicationDescription("Polygon triangulation with Seidel's algorithm");
	auto optHelp = argParser.addHelpOption();
	argParser.addPositionalArgument("input_file", "Input file with polygon data (.poly)");

	QCommandLineOption optBmark { QStringList() << "b" << "bmark", "benchmark triangulation" };
	argParser.addOption(optBmark);

	QCommandLineOption optIter { QStringList() << "i" << "iterations", "benchmark iterations", "n", "1" };
	argParser.addOption(optIter);

	argParser.process(a);

	if (argParser.isSet(optHelp))
	{
		//printf(argParser.helpText().toStdString().c_str());
		QTextStream ts { stdout };
		ts << argParser.helpText() << "\n";
	}
	else if (argParser.isSet(optBmark))
	{
		auto posArgs = argParser.positionalArguments();
		auto strNumIter = argParser.value(optIter);
		bool ok = false;
		int numIter = strNumIter.toInt(&ok);
		SeidelVisualize::Benchmark(posArgs[0], ok ? numIter : 1);
		return 0;
	}
	else
	{
		SeidelVisualize w;
		w.show();
		return a.exec();
	}
}
