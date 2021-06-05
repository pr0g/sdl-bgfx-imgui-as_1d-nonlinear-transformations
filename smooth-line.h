#pragma once

#include "thh-bgfx-debug/debug-line.hpp"

namespace dbg
{

struct SmoothLine
{
  DebugLines debug_lines_;

public:
  void setRenderContext(bgfx::ViewId view, bgfx::ProgramHandle program_handle);
  void draw(const as::vec3& begin, const as::vec3& end);
};

} // namespace dbg
