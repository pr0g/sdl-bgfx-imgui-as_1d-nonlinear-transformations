#pragma once

#include <as/as-math-ops.hpp>

#include <cassert>
#include <functional>
#include <string>
#include <vector>

enum class direction_e { vertical, horizontal };

struct bound_t {
  as::vec2i top_left_;
  as::vec2i bottom_right_;
};

struct list_t {
  as::vec2i position_;
  as::vec2i item_size_;
  int32_t spacing_ = 5;
  direction_e direction_;
  int32_t wrap_count_ = 1;

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
void press_list(list_t& list, const as::vec2i& mouse_position);
void move_list(list_t& list, const as::vec2i& movement_delta);

bound_t calculate_bound(const list_t& list, const int32_t index);
bound_t calculate_drag_bound(const list_t& list);

template<typename Item>
void release_list(list_t& list) {
  auto* items = static_cast<Item*>(list.items_);
  const int32_t begin = std::min(list.available_index_, list.selected_index_);
  const int32_t end = std::max(list.available_index_, list.selected_index_) + 1;
  const int32_t pivot = list.selected_index_ < list.available_index_
                        ? list.selected_index_ + 1
                        : list.selected_index_;
  std::rotate(items + begin, items + pivot, items + end);

  list.selected_index_ = -1;
  list.available_index_ = -1;
  list.drag_position_ = as::vec2i::zero();
}

inline direction_e invert_direction(const direction_e direction) {
  switch (direction) {
    case direction_e::horizontal:
      return direction_e::vertical;
    case direction_e::vertical:
      return direction_e::horizontal;
    default:
      assert(false);
      return direction_e::vertical;
  }
}

inline as::vec2i direction_mask(const direction_e direction) {
  switch (direction) {
    case direction_e::horizontal:
      return as::vec2i::axis_x();
    case direction_e::vertical:
      return as::vec2i::axis_y();
    default:
      assert(false);
      return as::vec2i::zero();
  }
}

inline as::vec2i invert_direction_mask(const direction_e direction) {
  return as::vec2i::one() - direction_mask(direction);
}
