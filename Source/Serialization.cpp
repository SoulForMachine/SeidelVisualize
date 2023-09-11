#include "Serialization.h"
#include <fstream>
#include <sstream>

bool LoadPolyFile(const std::string& polyFile, OutlineList& outlines)
{
	std::ifstream file(polyFile);
	if (!file.is_open())
		return false;

	Outline points;
	std::string line;
	while (std::getline(file, line))
	{
		if (line == "*")
		{
			if (!points.empty())
				outlines.push_back(std::move(points));
		}
		else
		{
			auto strList = SplitString(line);
			if (strList.size() == 2)
			{
				try
				{
					float valX = std::stof(strList[0]);
					float valY = std::stof(strList[1]);
					points.push_back({ valX, valY });
				}
				catch (...) { }
			}
		}
	}

	if (!points.empty())
		outlines.push_back(std::move(points));

	return true;
}

bool SavePolyFile(const std::string& polyFile, const OutlineList& outlines)
{
	std::ofstream file(polyFile);
	if (!file.is_open())
		return false;

	for (index_t i = 0; i < outlines.size(); ++i)
	{
		if (i > 0)
			file << "*\n";

		auto& outl = outlines[i];
		for (auto& pt : outl)
		{
			file << pt.x << " " << pt.y << "\n";
		}
	}

	return true;
}

bool SaveTriangleIndices(const std::string& triangleFile, const IndexList& indices)
{
	std::ofstream file(triangleFile);
	if (!file.is_open())
		return false;

	for (index_t i = 0; i < indices.size(); i += 3)
	{
		file << indices[i] << " " << indices[i + 1] << " " << indices[i + 2] << "\n";
	}

	return true;
}

bool SaveTrianglePoints(const std::string& triangleFile, const IndexList& indices, const std::vector<math3d::vec2f>& pointCoords)
{
	std::ofstream file(triangleFile);
	if (!file.is_open())
		return false;

	for (index_t i = 0; i < indices.size(); i += 3)
	{
		file << "[" << pointCoords[indices[i]].x << " " << pointCoords[indices[i]].y << "] "
			<< "[" << pointCoords[indices[i + 1]].x << " " << pointCoords[indices[i + 1]].y << "] "
			<< "[" << pointCoords[indices[i + 2]].x << " " << pointCoords[indices[i + 2]].y << "]\n";
	}

	return true;
}
