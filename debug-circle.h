#pragma once

#include "as/as-math-ops.hpp"
#include "debug-lines.h"

namespace dbg
{

class DebugCircles
{
  static DebugVertex CircleVertices[];
  static uint16_t CircleIndices[];

  bgfx::VertexBufferHandle circle_vbh_;
  bgfx::IndexBufferHandle circle_ibh_;
  bgfx::ProgramHandle program_handle_;
  bgfx::ViewId view_;

  struct CircleInstance
  {
    CircleInstance() = default;
    CircleInstance(const as::mat4& transform, const as::vec4& color)
      : transform_(transform), color_(color)
    {
    }

    as::mat4 transform_;
    as::vec4 color_;
  };

  std::vector<CircleInstance> instances_;

public:
  static void init();
  
  DebugCircles(bgfx::ViewId view, bgfx::ProgramHandle program_handle);
  ~DebugCircles();

  void reserveCircles(size_t count);
  void addCircle(const as::mat4& transform, const as::vec4& color);
  void submit();
};

inline void DebugCircles::addCircle(
  const as::mat4& transform, const as::vec4& color)
{
  instances_.emplace_back(transform, color);
}

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
      const float vertical_position =
        std::sin(current_vertical_angle_rad) * size;
      const float horizontal_position =
        std::cos(current_vertical_angle_rad) * size;
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

  void setTransform(const as::mat4& transform) { transform_ = transform; }

  void draw() const
  {
    debug_lines_.setTransform(transform_);
    debug_lines_.submit();
  }
};

} // namespace dbg
