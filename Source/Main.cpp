#include <exception>
#include <iostream>
#include <string>
#include <cstring>
#include <GLFW/glfw3.h>
#include "MainWindow.h"
#include "Benchmark.h"


int RunGUI()
{
	try
	{
		nanogui::init();

		{
			MainWindow mainWnd;
			mainWnd.set_visible(true);
			glfwMaximizeWindow(mainWnd.glfw_window());
			mainWnd.draw_all();
			nanogui::mainloop();
		}

		nanogui::shutdown();
	}
	catch (const std::exception& ex)
	{
		std::cout << "Exception: " << ex.what() << "\n";
		return -1;
	}

	return 0;
}

void DoBenchmark(const char* polygonFileName, int numIter)
{
	Benchmark bmark;
	std::string errDesc;
	if (bmark.LoadPolygon(polygonFileName, errDesc))
	{
		Benchmark::Statistics stats;
		bmark.Run(numIter, stats);
		std::cout
			<< "Finished in " << stats.totalTimeMS << " ms\n"
			<< "Number of outlines: " << stats.numOutlines << "\n"
			<< "Total number of points: " << stats.numPoints << "\n"
			<< "Average algorithm time: " << stats.averageTimeMS << " ms\n";
	}
	else
	{
		std::cout << "Error: " << errDesc << "\n";
	}
}

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		return RunGUI();
	}
	else if (argc == 4 && std::strncmp(argv[1], "-b", 3) == 0)
	{
		int iters = 0;
		try
		{
			iters = std::stoi(argv[3]);
		}
		catch (const std::exception&)
		{
			std::cout << "Wrong \"number of iterations\" parameter.\n";
			return -1;
		}

		DoBenchmark(argv[2], iters);
	}
	else
	{
		std::cout
			<< "Wrong command line arguments.\n"
			<< "Supply no arguments to run the GUI.\n"
			<< "To run a benchmark: SeidelVisualize -b <polygon file> <number of iterations>\n";

		return -1;
	}
}
