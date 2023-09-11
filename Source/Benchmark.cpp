#include "Benchmark.h"
#include <chrono>
#include <vector>
#include "Serialization.h"

bool Benchmark::LoadPolygon(const char* polygonFileName, std::string& errDesc)
{
	OutlineList outlines;
	if (!LoadPolyFile(polygonFileName, outlines))
	{
		errDesc = "Failed to load polygon file.";
		return false;
	}

	_triangulator = std::make_unique<SeidelTriangulator>(outlines);
	if (!_triangulator->IsSimplePolygon())
	{
		_triangulator.reset();
		errDesc = "Not a simple polygon.";
		return false;
	}

	return true;
}

void Benchmark::Run(int numIterations, Statistics& statistics)
{
	if (numIterations <= 0 || _triangulator == nullptr)
	{
		statistics = { };
		return;
	}

	SeidelTriangulator::TrapezoidationInfo trapInfo { };
	SeidelTriangulator::TriangulationInfo triangInfo { };
	IndexList triangleIndices;
	IndexList diagonalIndices;
	std::vector<IndexList> monotoneChains;

	statistics.numOutlines = _triangulator->GetOutlinesWinding().size();
	statistics.numPoints = _triangulator->GetPointCoords().size();
	statistics.averageTimeMS = 0.0f;

	auto startTime = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < numIterations; ++i)
	{
		auto iterStartTime = std::chrono::high_resolution_clock::now();

		_triangulator->BuildTrapezoidTree(trapInfo);
		_triangulator->Triangulate(triangInfo, triangleIndices, diagonalIndices, monotoneChains);

		auto iterEndTime = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration<double, std::chrono::milliseconds::period>(iterEndTime - iterStartTime).count();
		statistics.averageTimeMS += time / numIterations;
	}

	auto endTime = std::chrono::high_resolution_clock::now();
	statistics.totalTimeMS = std::chrono::duration<double, std::chrono::milliseconds::period>(endTime - startTime).count();
}
