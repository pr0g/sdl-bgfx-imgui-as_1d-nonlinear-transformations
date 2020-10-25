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

class DebugQuads
{
  static const DebugVertex QuadVertices[];
  static const uint16_t QuadIndices[];

  bgfx::VertexBufferHandle quad_vbh_;
  bgfx::IndexBufferHandle quad_ibh_;
  bgfx::ProgramHandle program_handle_;
  bgfx::ViewId view_;

  struct QuadInstance
  {
    QuadInstance() = default;
    QuadInstance(const as::mat4& transform, const as::vec4 color)
      : transform_(transform), color_(color)
    {
    }

    as::mat4 transform_;
    as::vec4 color_;
  };

  std::vector<QuadInstance> instances_;

public:
  DebugQuads(bgfx::ViewId view, bgfx::ProgramHandle program_handle);
  ~DebugQuads();

  void reserveQuads(size_t count);
  void addQuad(const as::mat4& transform, const as::vec4& color);
  void submit();
};

inline void DebugQuads::addQuad(
  const as::mat4& transform, const as::vec4& color)
{
  instances_.emplace_back(transform, color);
}

class DebugLines
{
  as::mat4 transform_ = as::mat4::identity();
  bgfx::ProgramHandle program_handle_;
  bgfx::ViewId view_;
  std::vector<DebugLine> lines_;

public:
  DebugLines(const bgfx::ViewId view, const bgfx::ProgramHandle program_handle)
    : view_(view), program_handle_(program_handle)
  {
  }

  void setTransform(const as::mat4& transform) { transform_ = transform; }
  void addLine(const as::vec3& begin, const as::vec3& end, uint32_t col);
  void submit();
};

} // namespace dbg
