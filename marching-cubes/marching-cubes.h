#pragma once

#include "as/as-math-ops.hpp"

#include <vector>

namespace mc
{

struct Point
{
  float val_;
  as::vec3 position_;
  as::vec3 normal_;
};

struct CellValues
{
  float values_[8];
};

struct CellPositions
{
  as::vec3 points_[8];
  as::vec3 normals_[8];
};

struct Triangle
{
  Triangle() = default;
  Triangle(
    const as::vec3& a, const as::vec3& b, const as::vec3& c, const as::vec3& an,
    const as::vec3& bn, const as::vec3& cn)
    : verts_{a, b, c}, norms_{an, bn, cn}
  {
  }

  as::vec3 verts_[3];
  as::vec3 norms_[3];
};

Point*** createPointVolume(int dimension, float initial_values);
CellValues*** createCellValues(int dimension);
CellPositions*** createCellPositions(int dimension);

void generatePointData(
  Point*** points, int dimension, float scale, float tesselation,
  const as::vec3& cam);

void generatePointData(
  Point*** points, int dimension, float tesselation, const as::vec3& center,
  const as::vec3& cam, const as::vec3& dir, float distance);

void generateCellData(
  CellPositions*** cell_positions, CellValues*** cell_values, Point*** points,
  int dimension);

void destroyPointVolume(Point*** points, int dimension);

void destroyCellValues(CellValues*** cells, int dimension);
void destroyCellPositions(CellPositions*** cells, int dimension);

std::vector<Triangle> march(
  CellPositions*** cell_positions, CellValues*** cell_values, int dimension,
  float threshold);

} // namespace mc
