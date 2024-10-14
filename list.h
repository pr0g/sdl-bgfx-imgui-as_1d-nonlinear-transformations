#pragma once

#include <as/as-math-ops.hpp>

#include <functional>
#include <vector>

struct bound_t
{
  as::vec2 top_left_;
  as::vec2 bottom_right_;
};

struct item_t
{
  uint32_t color_;
};

struct list_t
{
  as::vec2 position_;
  as::vec2 item_size_;
  float vertical_spacing_ = 5.0f;
  std::vector<item_t> items_;

  int32_t selected_index_ = -1;
  int32_t available_index_ = -1;
  as::vec2 drag_position_;
};

using draw_box_fn = std::function<void(
  const as::vec2& position, const as::vec2& size, const uint32_t color)>;

void update_list(list_t& list, const draw_box_fn& draw_box);

void press_list(list_t& list, const as::vec2i& mouse_position);
void release_list(list_t& list);
void move_list(list_t& list, float movement_delta);
