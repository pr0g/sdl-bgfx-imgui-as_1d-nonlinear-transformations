#pragma once

#include "scene.h"

#include <as-camera/as-camera.hpp>
#include <bgfx/bgfx.h>
#include <thh-bgfx-debug/debug-shader.hpp>

struct simple_camera_scene_t : public scene_t
{
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override {}
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override {}

  bgfx::ViewId main_view_;

  as::mat4 perspective_projection_;
  asc::Camera camera_;
};
