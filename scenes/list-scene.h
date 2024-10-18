#pragma once

#include "list.h"
#include "scene.h"

#include <as-camera/as-camera.hpp>
#include <bgfx/bgfx.h>
#include <thh-bgfx-debug/debug-color.hpp>

const inline auto white = dbg::encodeColorAbgr((uint8_t)255, 255, 255, 255);
const inline auto red = dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255);
const inline auto green = dbg::encodeColorAbgr((uint8_t)0, 255, 0, 255);
const inline auto blue = dbg::encodeColorAbgr((uint8_t)0, 0, 255, 255);
const inline auto yellow = dbg::encodeColorAbgr((uint8_t)255, 255, 0, 255);
const inline auto cyan = dbg::encodeColorAbgr((uint8_t)0, 255, 255, 255);
const inline auto light_coral = dbg::encodeColorAbgr((uint8_t)205, 92, 92, 255);
const inline auto pale_violet_red =
  dbg::encodeColorAbgr((uint8_t)219, 112, 147, 255);
const inline auto tomato = dbg::encodeColorAbgr((uint8_t)255, 99, 71, 255);
const inline auto orange = dbg::encodeColorAbgr((uint8_t)255, 165, 0, 255);
const inline auto khaki = dbg::encodeColorAbgr((uint8_t)240, 230, 140, 255);
const inline auto pale_green =
  dbg::encodeColorAbgr((uint8_t)152, 251, 152, 255);

struct item_t {
  uint32_t color_;
  std::string name_;
};

struct list_scene_t : public scene_t {
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override {}

  bgfx::ViewId ortho_view_;

  as::mat4 orthographic_projection_;
  asc::Camera camera_;

  list_t vertical_list_;
  list_t horizontal_list_;
  std::vector<item_t> items_;

  as::vec2i mouse_now_;
  as::vec2i screen_dimension_;
};
