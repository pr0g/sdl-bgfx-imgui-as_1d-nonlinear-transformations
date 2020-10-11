#pragma once

#include "debug-lines.h"

namespace dbg
{

struct SmoothLine
{
  DebugLines debug_lines_;

public:
  SmoothLine(const bgfx::ViewId view, const bgfx::ProgramHandle program_handle)
    : debug_lines_(view, program_handle)
  {
  }

  void draw(const as::vec3& begin, const as::vec3& end);
};

} // namespace dbg
