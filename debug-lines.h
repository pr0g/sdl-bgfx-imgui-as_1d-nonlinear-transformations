#pragma once

#include "as/as-math-ops.hpp"
#include "bgfx/bgfx.h"

#include <vector>

namespace dbg
{

struct DebugVertex
{
  DebugVertex(const as::vec3& position, const uint32_t color)
    : position_(position), color_(color)
  {
  }

  static void init();

  as::vec3 position_;
  uint32_t color_;

  static inline bgfx::VertexLayout Layout;
};

struct DebugLine
{
  DebugLine(const as::vec3& begin, const as::vec3& end, const uint32_t col)
    : begin_(begin, col), end_(end, col)
  {
  }

  DebugVertex begin_;
  DebugVertex end_;
};

class DebugLines
{
public:
  DebugLines(const bgfx::ViewId view, const bgfx::ProgramHandle programHandle)
    : view_(view), programHandle_(programHandle)
  {
  }

  void setTransform(const as::mat4& transform)
  {
    transform_ = transform;
  }

  void addLine(const as::vec3& begin, const as::vec3& end, const uint32_t col);
  void submit();

private:
  as::mat4 transform_ = as::mat4::identity();
  bgfx::ProgramHandle programHandle_;
  bgfx::ViewId view_;
  std::vector<DebugLine> lines_;
};

} // namespace dbg
