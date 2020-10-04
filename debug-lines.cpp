#include "debug-lines.h"

namespace dbg
{

void DebugVertex::init()
{
  Layout.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();
}

void DebugLines::addLine(
  const as::vec3& begin, const as::vec3& end, const uint32_t col)
{
  lines_.emplace_back(begin, end, col);
}

void DebugLines::submit()
{
  const auto requested_vertex_count = lines_.size() * 2;
  if (requested_vertex_count == 0) {
    return;
  }

  const auto available_vertex_count = bgfx::getAvailTransientVertexBuffer(
    requested_vertex_count, DebugVertex::Layout);
  if (available_vertex_count != requested_vertex_count) {
    return;
  }

  bgfx::TransientVertexBuffer line_tvb;
  bgfx::allocTransientVertexBuffer(
    &line_tvb, requested_vertex_count, DebugVertex::Layout);

  std::memcpy(
    line_tvb.data, lines_.data(),
    DebugVertex::Layout.getStride() * requested_vertex_count);

  float transform[16];
  as::mat_to_arr(transform_, transform);
  bgfx::setTransform(transform);

  bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);

  bgfx::setVertexBuffer(0, &line_tvb, 0, requested_vertex_count);
  bgfx::submit(view_, program_handle_);
}

} // namespace dbg
