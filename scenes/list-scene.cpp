#include "list-scene.h"

#include <as/as-view.hpp>
#include <thh-bgfx-debug/debug-circle.hpp>
#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-line.hpp>

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <easy_iterator.h>
#include <imgui.h>

void draw_box(
  debug_draw_t& debug_draw, const bound_t& bounds, const uint32_t color,
  const std::string& name) {
  // top
  debug_draw.debug_lines_screen->addLine(
    as::vec3_from_vec2(as::vec2_from_vec2i(bounds.top_left_)),
    as::vec3(bounds.bottom_right_.x, bounds.top_left_.y, 0.0f), color);

  // bottom
  debug_draw.debug_lines_screen->addLine(
    as::vec3(bounds.top_left_.x, bounds.bottom_right_.y, 0.0f),
    as::vec3(bounds.bottom_right_.x, bounds.bottom_right_.y, 0.0f), color);

  // left
  debug_draw.debug_lines_screen->addLine(
    as::vec3_from_vec2(as::vec2_from_vec2i(bounds.top_left_)),
    as::vec3(bounds.top_left_.x, bounds.bottom_right_.y, 0.0f), color);

  // right
  debug_draw.debug_lines_screen->addLine(
    as::vec3(bounds.bottom_right_.x, bounds.top_left_.y, 0.0f),
    as::vec3_from_vec2(as::vec2_from_vec2i(bounds.bottom_right_)), color);

  ImDrawList* drawList = ImGui::GetBackgroundDrawList();
  drawList->AddText(
    ImVec2(bounds.top_left_.x, bounds.top_left_.y),
    IM_COL32(255, 255, 255, 255), name.c_str());
}

void draw_box(
  debug_draw_t& debug_draw, const as::vec2i& position, const as::vec2i& size,
  const uint32_t color, const std::string& name) {
  draw_box(
    debug_draw,
    bound_t{.top_left_ = position, .bottom_right_ = position + size}, color,
    name);
}

void list_scene_t::setup(
  bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
  uint16_t height) {
  ortho_view_ = ortho_view;
  orthographic_projection_ =
    as::ortho_metal_lh(0.0f, width, height, 0.0f, 0.0f, 1.0f);
  screen_dimension_ = as::vec2i(width, height);

  items_ = std::vector<item_t>{
    {.color_ = white, .name_ = "Item 1"},  {.color_ = red, .name_ = "Item 2"},
    {.color_ = green, .name_ = "Item 3"},  {.color_ = blue, .name_ = "Item 4"},
    {.color_ = yellow, .name_ = "Item 5"}, {.color_ = cyan, .name_ = "Item 6"},
  };

  vertical_list_.items_ = items_.data();
  vertical_list_.item_count_ = items_.size();
  vertical_list_.item_stride_ = sizeof(item_t);
  vertical_list_.position_ = as::vec2i(200, 200);
  vertical_list_.item_size_ = as::vec2i(200, 50);
  vertical_list_.direction_ = direction_e::vertical;
}

void list_scene_t::input(const SDL_Event& current_event) {
  if (current_event.type == SDL_MOUSEBUTTONDOWN) {
    SDL_MouseButtonEvent* mouse_button = (SDL_MouseButtonEvent*)&current_event;
    if (mouse_button->button == SDL_BUTTON_LEFT) {
      press_list(vertical_list_, mouse_now_);
    }
  }

  if (current_event.type == SDL_MOUSEBUTTONUP) {
    SDL_MouseButtonEvent* mouse_button = (SDL_MouseButtonEvent*)&current_event;
    if (mouse_button->button == SDL_BUTTON_LEFT) {
      release_list<item_t>(vertical_list_);
    }
  }

  if (current_event.type == SDL_MOUSEMOTION) {
    SDL_MouseMotionEvent* mouse_motion = (SDL_MouseMotionEvent*)&current_event;
    const as::vec2i mouse_now = as::vec2i(mouse_motion->x, mouse_motion->y);
    const as::vec2i mouse_delta = mouse_now - mouse_now_;
    move_list(vertical_list_, mouse_delta.y);
    mouse_now_ = mouse_now;
  }
}

void list_scene_t::update(debug_draw_t& debug_draw, float delta_time) {
  bgfx::dbgTextClear();

  float view_o[16];
  as::mat_to_arr(as::mat4::identity(), view_o);

  float proj_o[16];
  as::mat_to_arr(orthographic_projection_, proj_o);
  bgfx::setViewTransform(ortho_view_, view_o, proj_o);

  ImGui::Begin(
    "Invisible Window", nullptr,
    ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration
      | ImGuiWindowFlags_NoInputs);
  ImGui::SetWindowPos(ImVec2(0, 0));
  ImGui::SetWindowSize(ImGui::GetIO().DisplaySize);

  update_list(
    vertical_list_,
    [&debug_draw](
      const as::vec2i& position, const as::vec2i& size, const void* item) {
      const auto* list_item = static_cast<const item_t*>(item);
      draw_box(debug_draw, position, size, list_item->color_, list_item->name_);
    });

  ImGui::End();

  // display logical mouse position
  debug_draw.debug_circles_screen->addWireCircle(
    as::mat4_from_mat3_vec3(
      as::mat3_scale(5.0f),
      as::vec3_from_vec2(as::vec2_from_vec2i(mouse_now_))),
    0xffffffff);
}
