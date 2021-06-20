#pragma once

#include "thh-bgfx-debug/debug-line.hpp"

namespace dbg
{

struct SmoothLine
{
  DebugLines* debug_lines_ = nullptr;

public:
  explicit SmoothLine(DebugLines* debug_lines) : debug_lines_(debug_lines) {}
  void draw(const as::vec3& begin, const as::vec3& end);
};

} // namespace dbg
