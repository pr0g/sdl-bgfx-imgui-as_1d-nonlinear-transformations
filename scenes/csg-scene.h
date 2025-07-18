#pragma once

#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>
#include <as/as-math-ops.hpp>
#include <bgfx/bgfx.h>

#include <array>
#include <vector>

struct csg_scene_t : public scene_t {
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override {}

  as::vec2i screen_dimension_;
  as::mat4 perspective_projection_;

  asc::Camera camera_;
  asc::Camera target_camera_;
  asci::CameraSystem camera_system_;
  asci::RotateCameraInput orbit_rotate_camera_{asci::MouseButton::Left};
  asci::PivotDollyScrollCameraInput orbit_dolly_camera_{};

  bgfx::ViewId main_view_;
  bgfx::ViewId ortho_view_;
};
