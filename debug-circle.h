#pragma once

#include "as/as-math-ops.hpp"
#include "debug-lines.h"

namespace dbg
{

class DebugCircle
{
  mutable dbg::DebugLines debugLines_;
  as::mat4 transform_;

public:
  DebugCircle(
    const as::mat4& transform, const float size, const bgfx::ViewId view,
    const bgfx::ProgramHandle programHandle)
    : debugLines_(view, programHandle), transform_(transform)
  {
    float rot = 0.0f;
    const float increment = (as::kPi * 2.0f) / 20.0f;
    for (size_t i = 0; i < 20; ++i) {
      debugLines_.addLine(
        as::vec3(std::cos(rot), std::sin(rot), 0.0f) * size,
        as::vec3(std::cos(rot + increment), std::sin(rot + increment), 0.0f)
          * size,
        0xff000000);
      rot += increment;
    }
  }

  void draw() const
  {
    debugLines_.setTransform(transform_);
    debugLines_.submit();
  }
};

} // namespace dbg
