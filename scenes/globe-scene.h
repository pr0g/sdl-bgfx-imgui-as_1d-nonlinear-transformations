#pragma once

#include "scene.h"

#include <as-camera/as-camera.hpp>
#include <bgfx/bgfx.h>

struct globe_scene_t : public scene_t
{
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override {}
};
