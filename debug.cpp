#include "debug.h"

#include <easy_iterator.h>
#include <thh-bgfx-debug/debug-line.hpp>

void drawTransform(
  dbg::DebugLines& debug_lines, const as::affine& affine,
  const float alpha /*= 1.0f*/)
{
  const auto alpha_8 = static_cast<uint8_t>(255.0f * alpha);
  const uint32_t alpha_32 = alpha_8 << 24;

  const auto& translation = affine.translation;
  debug_lines.addLine(
    translation, translation + as::mat3_basis_x(affine.rotation),
    0x000000ff | alpha_32);
  debug_lines.addLine(
    translation, translation + as::mat3_basis_y(affine.rotation),
    0x0000ff00 | alpha_32);
  debug_lines.addLine(
    translation, translation + as::mat3_basis_z(affine.rotation),
    0x00ff0000 | alpha_32);
}

void drawGrid(dbg::DebugLines& debug_lines, const as::vec3& position)
{
  const auto grid_scale = 10.0f;
  const auto grid_camera_offset = as::vec_snap(position, grid_scale);
  const auto grid_dimension = 20;
  const auto grid_size = static_cast<float>(grid_dimension) * grid_scale;
  const auto grid_offset = grid_size * 0.5f;
  namespace ei = easy_iterator;
  for (auto line : ei::range(grid_dimension + 1)) {
    const auto start = (static_cast<float>(line) * grid_scale) - grid_offset;
    const auto flattened_offset =
      as::vec3(grid_camera_offset.x, 0.0f, grid_camera_offset.z);
    debug_lines.addLine(
      as::vec3(-grid_offset, 0.0f, start) + flattened_offset,
      as::vec3(0.0f + grid_offset, 0.0f, start) + flattened_offset, 0xff000000);
    debug_lines.addLine(
      as::vec3(start, 0.0f, -grid_offset) + flattened_offset,
      as::vec3(start, 0.0f, grid_offset) + flattened_offset, 0xff000000);
  }
}
