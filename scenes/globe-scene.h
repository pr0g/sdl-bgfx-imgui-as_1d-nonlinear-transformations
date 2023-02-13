#pragma once

#include "bgfx-helpers.h"
#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>
#include <bgfx/bgfx.h>

#include <vector>

struct globe_scene_t : public scene_t
{
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override {}

  std::vector<PosColorVertex> sphere_vertices;
  std::vector<uint16_t> sphere_indices;

  bgfx::VertexLayout pos_col_vert_layout_;
  bgfx::VertexBufferHandle sphere_col_vbh_;
  bgfx::IndexBufferHandle sphere_col_ibh_;
  bgfx::ProgramHandle program_col_;

  as::mat4 perspective_projection_;
  asc::Camera camera_;
  asc::Camera target_camera_;

  asci::RotateCameraInput first_person_rotate_camera_{asci::MouseButton::Right};
  asci::PivotDollyScrollCameraInput first_person_dolly_camera_;

  asci::CameraSystem camera_system_;

  bgfx::ViewId main_view_;

  as::vec2i screen_dimension_;

  bool rotating_ = false;
  as::vec2 roll_pitch_ = as::vec2::zero();
  as::mat3 model_ = as::mat3::identity();
};
