#pragma once

#include "scene.h"

#include <as-camera/as-camera.hpp>
#include <bgfx/bgfx.h>

#include <string>
#include <vector>

struct item_t
{
  uint32_t color_;
};

struct list_t
{
  as::vec2 position_;
  as::vec2 item_size_;
  float spacing_ = 5.0f;
  std::vector<item_t> items_;
};

struct list_scene_t : public scene_t
{
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override {}

  bgfx::ViewId ortho_view_;

  as::mat4 orthographic_projection_;
  asc::Camera camera_;

  list_t list_;

  as::vec2i mouse_now_;
  as::vec2i screen_dimension_;

  int32_t selected_index_ = -1;
  int32_t available_index_ = -1;
  as::vec2 drag_position_;
};
