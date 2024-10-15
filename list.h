#pragma once

#include <as/as-math-ops.hpp>

#include <functional>
#include <string>
#include <vector>

struct bound_t
{
  as::vec2i top_left_;
  as::vec2i bottom_right_;
};

struct item_t
{
  uint32_t color_;
  std::string name_;
};

struct list_t
{
  as::vec2i position_;
  as::vec2i item_size_;
  float vertical_spacing_ = 5.0f;

  void* items_;
  int32_t item_count_;
  int32_t item_stride_;

  int32_t selected_index_ = -1;
  int32_t available_index_ = -1;
  as::vec2i drag_position_;
};

using draw_box_fn = std::function<void(
  const as::vec2i& position, const as::vec2i& size, const void* item)>;
void update_list(list_t& list, const draw_box_fn& draw_box);

using reorder_fn = std::function<void(list_t& list)>;

void press_list(list_t& list, const as::vec2i& mouse_position);
void release_list(list_t& list, const reorder_fn& reorder);
void move_list(list_t& list, float movement_delta);

template<typename Item>
void reorder_list(list_t& list)
{
  auto* items = static_cast<Item*>(list.items_);
  const int32_t begin = std::min(list.available_index_, list.selected_index_);
  const int32_t end = std::max(list.available_index_, list.selected_index_) + 1;
  const int32_t pivot = list.selected_index_ < list.available_index_
                        ? list.selected_index_ + 1
                        : list.selected_index_;
  std::rotate(items + begin, items + pivot, items + end);
}
