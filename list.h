#pragma once

#include <as/as-math-ops.hpp>

#include <cassert>
#include <functional>
#include <optional>
#include <vector>

enum class order_e { top_to_bottom, left_to_right };

struct bound_t {
  as::vec2i top_left_;
  as::vec2i bottom_right_;
};

struct list_t {
  as::vec2i position_;
  as::vec2i item_size_;
  int32_t spacing_ = 5;
  order_e order_;
  int32_t wrap_count_ = 0;

  void* items_;
  int32_t item_count_;
  int32_t item_stride_;

  int32_t selected_index_ = -1;
  int32_t available_index_ = -1;
  as::vec2i drag_position_;
};

struct item_display_t {
  as::vec2i current_position_;
  as::vec2i start_position_;
  as::vec2i next_position_;
  int32_t current_index_;
  int32_t next_index_;
  std::optional<float> t_;
  float dir_ = 1.0f;
};

struct list_display_t {
  std::vector<item_display_t> items_;
};

as::vec2i item_offset(const list_t& list, int32_t index);

using draw_box_fn = std::function<void(
  const as::vec2i& position, const as::vec2i& size, const void* item)>;

void init_list(list_t& list, list_display_t& list_display);

void update_list(
  list_t& list, list_display_t& list_display, const draw_box_fn& draw_box,
  float delta_time);
void press_list(list_t& list, const as::vec2i& mouse_position);
void move_list(list_t& list, const as::vec2i& movement_delta);

template<typename Item>
void release_list(list_t& list, list_display_t& list_display) {
  auto* items = static_cast<Item*>(list.items_);
  const int32_t begin = std::min(list.available_index_, list.selected_index_);
  const int32_t end = std::max(list.available_index_, list.selected_index_) + 1;
  const int32_t pivot = list.selected_index_ < list.available_index_
                        ? list.selected_index_ + 1
                        : list.selected_index_;
  list_display.items_[list.selected_index_].current_index_ =
    list.available_index_;
  list_display.items_[list.selected_index_].next_index_ = list.available_index_;
  list_display.items_[list.selected_index_].next_position_ =
    list.position_ + item_offset(list, list.available_index_);
  list_display.items_[list.selected_index_].start_position_ =
    list.drag_position_;
  list_display.items_[list.selected_index_].t_ = 0.0f;
  std::rotate(items + begin, items + pivot, items + end);
  std::rotate(
    list_display.items_.begin() + begin, list_display.items_.begin() + pivot,
    list_display.items_.begin() + end);
  list.selected_index_ = -1;
  list.available_index_ = -1;
  list.drag_position_ = as::vec2i::zero();
}

bound_t calculate_bound(const list_t& list, int32_t index);
bound_t calculate_drag_bound(const list_t& list);

// returns the minor order for the current major order
inline order_e minor_order(const order_e order) {
  switch (order) {
    case order_e::left_to_right:
      return order_e::top_to_bottom;
    case order_e::top_to_bottom:
      return order_e::left_to_right;
    default:
      assert(false);
      return order_e::top_to_bottom;
  }
}

// returns a vec2 mask for the current major order
inline as::vec2i order_mask(const order_e order) {
  switch (order) {
    case order_e::left_to_right:
      return as::vec2i::axis_x();
    case order_e::top_to_bottom:
      return as::vec2i::axis_y();
    default:
      assert(false);
      return as::vec2i::zero();
  }
}

// returns a vec2 mask for the minor order given the current major order
inline as::vec2i minor_order_mask(const order_e order) {
  return as::vec2i::one() - order_mask(order);
}
