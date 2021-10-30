#pragma once

#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>
#include <as-camera/as-camera.hpp>
#include <bgfx/bgfx.h>
#include <thh-bgfx-debug/debug-shader.hpp>

struct voronoi_scene_t : public scene_t
{
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw) override;
  void teardown() override {}

  bgfx::ViewId main_view_;
  as::mat4 perspective_projection_;

  asc::Camera camera_;
  asc::Camera target_camera_;
  asci::RotateCameraInput first_person_rotate_camera_{asci::MouseButton::Right};
  asci::TranslateCameraInput first_person_translate_camera_{
    asci::lookTranslation, asci::translatePivot};
  asci::SmoothProps smooth_props_;
  asci::Cameras cameras_;
  asci::CameraSystem camera_system_;

  as::vec2 focus_ = as::vec2(0.0f, 5.0f);
  as::vec2 focus2_ = as::vec2(2.0f, 6.0f);
  as::vec2 focus3_ = as::vec2(-2.0f, 5.5f);

  float directrix_ = 4.5f;

  int64_t prev_;
};
