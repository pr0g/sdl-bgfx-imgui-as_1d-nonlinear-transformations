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

const as::vec2i item_offset(const list_t& list, const int32_t index) {
  const int32_t columns = 3;
  const int32_t row = index / columns;
  const int32_t col = index % columns;

  const int32_t horizontal = (list.item_size_.x + list.spacing_) * col;
  const int32_t vertical = (list.item_size_.y + list.spacing_) * row;

  return as::vec2i(horizontal, vertical);
}

bound_t calculate_bound(const list_t& list, const int32_t index) {
  const as::vec2i offset = item_offset(list, index);
  return bound_t{
    list.position_ + offset, list.position_ + list.item_size_ + offset};
}

bound_t calculate_drag_bound(const list_t& list) {
  return bound_t{list.drag_position_, list.drag_position_ + list.item_size_};
}

struct extent_t {
  int32_t min;
  int32_t max;
};

static extent_t item_extents(
  const direction_e direction, const bound_t& bound) {
  return {
    .min = as::vec_max_elem(bound.top_left_ * direction_mask(direction)),
    .max = as::vec_max_elem(bound.bottom_right_ * direction_mask(direction))};
}

static int32_t item_direction_dimension(const list_t& list) {
  return as::vec_max_elem(list.item_size_ * direction_mask(list.direction_));
}

void update_list(list_t& list, const draw_box_fn& draw_box) {
  const auto item_size = list.item_size_;
  const auto item_dimension = item_direction_dimension(list);
  const auto items = static_cast<std::byte*>(list.items_);
  if (list.selected_index_ != -1) {
    const bound_t dragged_item_bound = calculate_drag_bound(list);
    const extent_t primary_dragged_item_extent =
      item_extents(list.direction_, dragged_item_bound);
    const void* item = items + list.selected_index_ * list.item_stride_;
    draw_box(
      dragged_item_bound.top_left_,
      dragged_item_bound.bottom_right_ - dragged_item_bound.top_left_, item);

    // if (index_before / 3 < list.available_index_ / 3) {
    //   list.available_index_ -= 3;
    // }

    const extent_t secondary_dragged_item_extent =
      item_extents(direction_e::vertical, dragged_item_bound);

    if (const int32_t secondary_index_before = list.available_index_ - 3;
        secondary_index_before >= 0) {
      extent_t secondary_item_before_extent = item_extents(
        direction_e::vertical, calculate_bound(list, secondary_index_before));
      if (
        secondary_dragged_item_extent.min
        < secondary_item_before_extent.max - (item_dimension / 2)) {
        // item_before_extent.max -= item_dimension;
        list.available_index_ -= 3;
      }
    }

    if (const int32_t secondary_index_after = list.available_index_ + 3;
        secondary_index_after < list.item_count_) {
      extent_t secondary_item_after_extent = item_extents(
        direction_e::vertical, calculate_bound(list, secondary_index_after));
      if (
        secondary_dragged_item_extent.max
        >= secondary_item_after_extent.min + (item_dimension / 2)) {
        // item_before_extent.max -= item_dimension;
        list.available_index_ += 3;
      }
    }

    if (const int32_t primary_index_before = list.available_index_ - 1;
        primary_index_before >= 0
        && primary_index_before / 3 == list.available_index_ / 3) {
      extent_t primary_item_before_extent = item_extents(
        list.direction_, calculate_bound(list, primary_index_before));
      if (
        primary_dragged_item_extent.min
        < primary_item_before_extent.max - (item_dimension / 2)) {
        // item_before_extent.max -= item_dimension;
        list.available_index_--;
      }
    }

    // if (index_after / 3 > list.available_index_ / 3) {
    //   list.available_index_ += 3;
    // }

    if (const int32_t primary_index_after = list.available_index_ + 1;
        primary_index_after < list.item_count_
        && primary_index_after / 3 == list.available_index_ / 3) {
      extent_t primary_item_after_extent = item_extents(
        list.direction_, calculate_bound(list, primary_index_after));
      if (
        primary_dragged_item_extent.max
        >= primary_item_after_extent.min + (item_dimension / 2)) {
        // item_after_extent.min += item_dimension;
        list.available_index_++;
      }
    }
  }

  const auto list_position = list.position_;
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
      list_position + item_offset(list, index + offset), item_size, item);
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
      list.drag_position_ = list_position + item_offset(list, index);
    }
  }
}

void move_list(list_t& list, const as::vec2i& movement_delta) {
  if (list.selected_index_ != -1) {
    list.drag_position_ += movement_delta;
  }
}
