#ifndef _SERIALIZATION_H_
#define _SERIALIZATION_H_

#include <string>
#include "SeidelTriangulator.h"

bool LoadPolyFile(const std::string& polyFile, OutlineList& outlines);
bool SavePolyFile(const std::string& polyFile, const OutlineList& outlines);
bool SaveTriangleIndices(const std::string& triangleFile, const IndexList& indices);
bool SaveTrianglePoints(const std::string& triangleFile, const IndexList& indices, const std::vector<math3d::vec2f>& pointCoords);

#endif // _SERIALIZATION_H_
