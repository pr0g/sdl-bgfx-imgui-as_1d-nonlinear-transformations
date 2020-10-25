#pragma once

#include "as/as-math-ops.hpp"
#include "debug-lines.h"

namespace dbg
{

class DebugCircle
{
  mutable dbg::DebugLines debug_lines_;
  as::mat4 transform_;

public:
  DebugCircle(
    const as::mat4& transform, const float size, const bgfx::ViewId view,
    const bgfx::ProgramHandle program_handle)
    : debug_lines_(view, program_handle), transform_(transform)
  {
    float rot = 0.0f;
    const float increment = as::k_tau / 20.0f;
    for (size_t i = 0; i < 20; ++i) {
      debug_lines_.addLine(
        as::vec3(std::cos(rot), std::sin(rot), 0.0f) * size,
        as::vec3(std::cos(rot + increment), std::sin(rot + increment), 0.0f)
          * size,
        0xff000000);
      rot += increment;
    }
  }

  void draw() const
  {
    debug_lines_.setTransform(transform_);
    debug_lines_.submit();
  }
};

class DebugSphere
{
  mutable dbg::DebugLines debug_lines_;
  as::mat4 transform_;

public:
  DebugSphere(
    const as::mat4& transform, const float size, const bgfx::ViewId view,
    const bgfx::ProgramHandle program_handle)
    : debug_lines_(view, program_handle), transform_(transform)
  {
    const size_t loops = 8;
    const float vertical_angle_inc_rad =
      as::radians(360.0f / (float(loops) * 2.0f));
    const float horizontal_angle_inc_rad = as::k_tau / 20.0f;
    const float starting_vertical_angle_rad = as::radians(90.0f);

    float current_vertical_angle_rad = starting_vertical_angle_rad;
    float current_horizontal_angle_rad = 0.0f;
    for (size_t loop = 0; loop < loops; ++loop) {
      const float vertical_position = std::sin(current_vertical_angle_rad);
      const float horizontal_position = std::cos(current_vertical_angle_rad);
      for (size_t segment_index = 0; segment_index < 20; ++segment_index) {
        debug_lines_.addLine(
          as::vec3(
            std::cos(current_horizontal_angle_rad) * horizontal_position,
            vertical_position,
            std::sin(current_horizontal_angle_rad) * horizontal_position),
          as::vec3(
            std::cos(current_horizontal_angle_rad + horizontal_angle_inc_rad)
              * horizontal_position,
            vertical_position,
            std::sin(current_horizontal_angle_rad + horizontal_angle_inc_rad)
              * horizontal_position),
          0xff000000);
        current_horizontal_angle_rad += horizontal_angle_inc_rad;
      }
      current_horizontal_angle_rad = 0.0f;
      current_vertical_angle_rad += vertical_angle_inc_rad;
    }
  }

  void draw() const
  {
    debug_lines_.setTransform(transform_);
    debug_lines_.submit();
  }
};

} // namespace dbg
