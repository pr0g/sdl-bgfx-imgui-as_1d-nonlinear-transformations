#pragma once

#include <as/as-math-ops.hpp>

namespace dbg
{
class DebugLines;
}

void drawTransform(
  dbg::DebugLines& debug_lines, const as::affine& affine,
  const float alpha = 1.0f);

void drawGrid(dbg::DebugLines& debug_lines, const as::vec3& position);
