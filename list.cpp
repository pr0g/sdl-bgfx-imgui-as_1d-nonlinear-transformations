#include "list.h"

// needed for debug_draw_t
#include "scenes/scene.h"

#include <as/as-math-ops.hpp>
#include <as/as-view.hpp>
#include <easy_iterator.h>

static bool contained(const bound_t& bound, const as::vec2i& point) {
  if (
    point.x >= bound.top_left_.x && point.x < bound.bottom_right_.x
    && point.y >= bound.top_left_.y && point.y < bound.bottom_right_.y) {
    return true;
  }
  return false;
}

bound_t calculate_bound(const list_t& list, const int32_t index) {
  const as::vec2i offset = direction_mask(list.direction_) * index
                         * (list.item_size_ + as::vec2i(list.spacing_));
  return bound_t{
    list.position_ + offset, list.position_ + list.item_size_ + offset};
}

bound_t calculate_drag_bound(const list_t& list) {
  const as::vec2i masked_drag_position =
    direction_mask(list.direction_) * list.drag_position_;
  return bound_t{
    invert_direction_mask(list.direction_) * list.position_
      + masked_drag_position,
    invert_direction_mask(list.direction_) * list.position_ + list.item_size_
      + masked_drag_position};
}

void update_list(list_t& list, const draw_box_fn& draw_box) {
  const auto item_size = list.item_size_;
  const auto list_position = list.position_;

  const auto items = static_cast<std::byte*>(list.items_);
  if (list.selected_index_ != -1) {
    const void* item = items + list.selected_index_ * list.item_stride_;
    draw_box(list.drag_position_, list.item_size_, item);

    const bound_t dragged_item_bound = calculate_drag_bound(list);
    if (const int32_t index_before = list.available_index_ - 1;
        index_before >= 0) {
      auto item_before_bound = calculate_bound(list, index_before);
      while (dragged_item_bound.top_left_.y
             < item_before_bound.bottom_right_.y - (item_size.y / 2)) {
        item_before_bound.bottom_right_.y -= item_size.y;
        list.available_index_--;
      }
    }

    if (const int32_t index_after = list.available_index_ + 1;
        index_after < list.item_count_) {
      auto item_after_bound = calculate_bound(list, index_after);
      while (dragged_item_bound.bottom_right_.y
             >= item_after_bound.top_left_.y + (item_size.y / 2)) {
        item_after_bound.top_left_.y += item_size.y;
        list.available_index_++;
      }
    }
  }

  for (int32_t index = 0; index < list.item_count_; index++) {
    const void* item = items + index * list.item_stride_;
    if (index == list.selected_index_) {
      continue;
    }
    int32_t offset = 0;
    if (index >= list.available_index_ && index < list.selected_index_) {
      offset = 1;
    }
    if (index <= list.available_index_ && index > list.selected_index_) {
      offset = -1;
    }
    draw_box(
      list_position
        + as::vec2i(0, (item_size.y + list.spacing_) * (index + offset)),
      item_size, item);
  }
}

void press_list(list_t& list, const as::vec2i& mouse_position) {
  const as::vec2i list_position = list.position_;
  const as::vec2i item_size = list.item_size_;
  for (int32_t index = 0; index < list.item_count_; index++) {
    const bound_t bound = calculate_bound(list, index);
    if (contained(bound, mouse_position)) {
      list.selected_index_ = index;
      list.available_index_ = index;
      list.drag_position_ =
        list_position + as::vec2i(0, index * (item_size.y + list.spacing_));
    }
  }
}

void move_list(list_t& list, const int32_t movement_delta) {
  if (list.selected_index_ != -1) {
    list.drag_position_ += direction_mask(list.direction_) * movement_delta;
  }
}
