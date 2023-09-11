#ifndef _BENCHMARK_H_
#define _BENCHMARK_H_

#include <string>
#include <memory>
#include "SeidelTriangulator.h"

class Benchmark
{
public:
	struct Statistics
	{
		int_t numOutlines = 0;
		int_t numPoints = 0;
		double totalTimeMS = 0.0f;
		double averageTimeMS = 0.0f;
	};

	bool LoadPolygon(const char* polygonFileName, std::string& errDesc);
	void Run(int numIterations, Statistics& statistics);

private:
	std::unique_ptr<SeidelTriangulator> _triangulator;
};

#endif // _BENCHMARK_H_
