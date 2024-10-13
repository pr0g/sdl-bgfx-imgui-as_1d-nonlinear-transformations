#include "list-scene.h"

#include <as/as-view.hpp>
#include <thh-bgfx-debug/debug-circle.hpp>
#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-line.hpp>

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <easy_iterator.h>

struct bound_t
{
  as::vec2 top_left_;
  as::vec2 bottom_right_;
};

bool contained(const bound_t& bound, const as::vec2& point)
{
  if (
    point.x >= bound.top_left_.x && point.x < bound.bottom_right_.x
    && point.y >= bound.top_left_.y && point.y < bound.bottom_right_.y) {
    return true;
  }
  return false;
}

void draw_box(
  debug_draw_t& debug_draw, const bound_t& bounds, const uint32_t color)
{
  // top
  debug_draw.debug_lines_screen->addLine(
    as::vec3_from_vec2(bounds.top_left_),
    as::vec3(bounds.bottom_right_.x, bounds.top_left_.y, 0.0f), color);

  // bottom
  debug_draw.debug_lines_screen->addLine(
    as::vec3(bounds.top_left_.x, bounds.bottom_right_.y, 0.0f),
    as::vec3(bounds.bottom_right_.x, bounds.bottom_right_.y, 0.0f), color);

  // left
  debug_draw.debug_lines_screen->addLine(
    as::vec3_from_vec2(bounds.top_left_),
    as::vec3(bounds.top_left_.x, bounds.bottom_right_.y, 0.0f), color);

  // right
  debug_draw.debug_lines_screen->addLine(
    as::vec3(bounds.bottom_right_.x, bounds.top_left_.y, 0.0f),
    as::vec3_from_vec2(bounds.bottom_right_), color);
}

void draw_box(
  debug_draw_t& debug_draw, const as::vec2& position, const as::vec2& bounds,
  const uint32_t color)
{
  const as::vec2 top_left = position;
  const as::vec2 bottom_right = position + bounds;
  draw_box(debug_draw, bound_t{top_left, bottom_right}, color);
}

void list_scene_t::setup(
  bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
  uint16_t height)
{
  ortho_view_ = ortho_view;
  orthographic_projection_ =
    as::ortho_metal_lh(0.0f, width, height, 0.0f, 0.0f, 1.0f);
  screen_dimension_ = as::vec2i(width, height);

  list_.items_ = std::vector<item_t>{
    {dbg::encodeColorAbgr((uint8_t)255, 255, 255, 255), as::vec2(200, 50)},
    {dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255), as::vec2(200, 50)},
    {dbg::encodeColorAbgr((uint8_t)0, 255, 0, 255), as::vec2(200, 50)},
    {dbg::encodeColorAbgr((uint8_t)0, 0, 255, 255), as::vec2(200, 50)},
    {dbg::encodeColorAbgr((uint8_t)255, 255, 0, 255), as::vec2(200, 50)},
    {dbg::encodeColorAbgr((uint8_t)0, 255, 255, 255), as::vec2(200, 50)},
  };
  list_.position_ = as::vec2(200, 200);
}

void list_scene_t::input(const SDL_Event& current_event)
{
  if (current_event.type == SDL_MOUSEBUTTONDOWN) {
    SDL_MouseButtonEvent* mouse_button = (SDL_MouseButtonEvent*)&current_event;
    if (mouse_button->button == SDL_BUTTON_LEFT) {
      namespace ei = easy_iterator;
      const as::vec2 list_position = list_.position_;
      for (const auto& [index, item] : ei::enumerate(list_.items_)) {
        const bound_t bound = bound_t{
          as::vec2(
            list_position.x,
            list_position.y + index * (item.size_.y + list_.spacing_)),
          as::vec2(
            list_position.x + item.size_.x,
            list_position.y + item.size_.y
              + index * (item.size_.y + list_.spacing_))};
        if (contained(bound, as::vec2_from_vec2i(mouse_now_))) {
          selected_index_ = index;
          available_index_ = index;
          drag_position_ =
            list_position
            + as::vec2(0.0f, index * (item.size_.y + list_.spacing_));
        }
      }
    }
  }

  if (current_event.type == SDL_MOUSEBUTTONUP) {
    SDL_MouseButtonEvent* mouse_button = (SDL_MouseButtonEvent*)&current_event;
    if (mouse_button->button == SDL_BUTTON_LEFT) {
      selected_index_ = -1;
      available_index_ = -1;
      drag_position_ = as::vec2::zero();
    }
  }

  if (current_event.type == SDL_MOUSEMOTION) {
    SDL_MouseMotionEvent* mouse_motion = (SDL_MouseMotionEvent*)&current_event;
    const as::vec2i mouse_now = as::vec2i(mouse_motion->x, mouse_motion->y);
    const as::vec2i mouse_delta = mouse_now - mouse_now_;
    if (selected_index_ != -1) {
      drag_position_.y += (float)mouse_delta.y;
    }
    mouse_now_ = mouse_now;
  }
}

void list_scene_t::update(debug_draw_t& debug_draw, float delta_time)
{
  bgfx::dbgTextClear();

  float view_o[16];
  as::mat_to_arr(as::mat4::identity(), view_o);

  float proj_o[16];
  as::mat_to_arr(orthographic_projection_, proj_o);
  bgfx::setViewTransform(ortho_view_, view_o, proj_o);

  namespace ei = easy_iterator;
  for (const auto& [index, item] : ei::enumerate(list_.items_)) {
    if (index == selected_index_) {
      continue;
    }
    int offset = 0;
    if (index >= available_index_ && index < selected_index_) {
      offset = 1;
    }
    if (index <= available_index_ && index > selected_index_) {
      offset = -1;
    }
    draw_box(
      debug_draw,
      list_.position_
        + as::vec2(0.0f, (item.size_.y + list_.spacing_) * (index + offset)),
      item.size_, item.color_);
  }

  const auto list_position = list_.position_;
  if (selected_index_ != -1) {
    const auto& item = list_.items_[selected_index_];
    draw_box(debug_draw, drag_position_, item.size_, item.color_);
    const bound_t item_bound = bound_t{
      as::vec2(list_position.x, drag_position_.y),
      as::vec2(
        list_position.x + item.size_.x, drag_position_.y + item.size_.y)};

    if (const int index_before = available_index_ - 1; index_before >= 0) {
      const auto& item_before = list_.items_[index_before];
      const auto position =
        list_.position_
        + as::vec2(0.0f, (item_before.size_.y + list_.spacing_) * index_before);
      const bound_t item_before_bound = bound_t{
        as::vec2(
          list_position.x,
          list_position.y
            + index_before * (item_before.size_.y + list_.spacing_)),
        as::vec2(
          list_position.x + item_before.size_.x,
          list_position.y + item_before.size_.y
            + index_before * (item_before.size_.y + list_.spacing_))};
      if (
        item_bound.top_left_.y
        < item_before_bound.bottom_right_.y - (item_before.size_.y / 2)) {
        available_index_--;
      }
    }

    if (const int index_after = available_index_ + 1;
        index_after < list_.items_.size()) {
      const auto& item_after = list_.items_[index_after];
      const auto position =
        list_.position_
        + as::vec2(0.0f, (item_after.size_.y + list_.spacing_) * index_after);
      const bound_t item_after_bound = bound_t{
        as::vec2(
          list_position.x,
          list_position.y
            + index_after * (item_after.size_.y + list_.spacing_)),
        as::vec2(
          list_position.x + item_after.size_.x,
          list_position.y + item_after.size_.y
            + index_after * (item_after.size_.y + list_.spacing_))};
      if (
        item_bound.bottom_right_.y
        >= item_after_bound.top_left_.y + (item_after.size_.y / 2)) {
        available_index_++;
      }
    }
  }

  // debug bounds drawing
  // const as::vec2 list_position = list_.position_;
  // for (const auto& [index, item] : ei::enumerate(list_.items_)) {
  //   const bound_t bound = bound_t{
  //     as::vec2(
  //       list_position.x,
  //       list_position.y + index * (item.bounds_.y + list_.spacing_)),
  //     as::vec2(
  //       list_position.x + item.bounds_.x,
  //       list_position.y + item.bounds_.y
  //         + index * (item.bounds_.y + list_.spacing_))};
  //   draw_box(debug_draw, bound, item.color_);
  // }

  debug_draw.debug_circles_screen->addWireCircle(
    as::mat4_from_mat3_vec3(
      as::mat3_scale(5.0f),
      as::vec3_from_vec2(as::vec2_from_vec2i(mouse_now_))),
    0xffffffff);
}
