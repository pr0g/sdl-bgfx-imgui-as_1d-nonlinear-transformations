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
  const int32_t major_index = index % list.wrap_count_;
  const int32_t minor_index = index / list.wrap_count_;
  const as::vec2i major =
    (order_mask(list.order_) * (list.item_size_ + as::vec2i(list.spacing_)))
    * major_index;
  const as::vec2i minor = (minor_order_mask(list.order_)
                           * (list.item_size_ + as::vec2i(list.spacing_)))
                        * minor_index;
  return major + minor;
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

static extent_t item_extents(const order_e order, const bound_t& bound) {
  return {
    .min = as::vec_max_elem(bound.top_left_ * order_mask(order)),
    .max = as::vec_max_elem(bound.bottom_right_ * order_mask(order))};
}

static int32_t item_order_dimension(const list_t& list) {
  return as::vec_max_elem(list.item_size_ * order_mask(list.order_));
}

static int32_t minor_item_order_dimension(const list_t& list) {
  return as::vec_max_elem(list.item_size_ * minor_order_mask(list.order_));
}

void update_list(list_t& list, const draw_box_fn& draw_box) {
  const auto item_major_dimension = item_order_dimension(list);
  const auto item_minor_dimension = minor_item_order_dimension(list);
  const auto items = static_cast<std::byte*>(list.items_);
  if (list.selected_index_ != -1) {
    const bound_t dragged_item_bound = calculate_drag_bound(list);
    const void* item = items + list.selected_index_ * list.item_stride_;
    draw_box(
      dragged_item_bound.top_left_,
      dragged_item_bound.bottom_right_ - dragged_item_bound.top_left_, item);

    const extent_t minor_dragged_item_extent =
      item_extents(minor_order(list.order_), dragged_item_bound);
    if (const int32_t minor_index_before =
          list.available_index_ - list.wrap_count_;
        minor_index_before >= 0) {
      extent_t minor_item_before_extent = item_extents(
        minor_order(list.order_), calculate_bound(list, minor_index_before));
      while (minor_dragged_item_extent.min
             < minor_item_before_extent.max - (item_minor_dimension / 2)) {
        minor_item_before_extent.max -= item_minor_dimension;
        list.available_index_ -= list.wrap_count_;
      }
    }

    if (const int32_t minor_index_after =
          list.available_index_ + list.wrap_count_;
        minor_index_after < list.item_count_) {
      extent_t minor_item_after_extent = item_extents(
        minor_order(list.order_), calculate_bound(list, minor_index_after));
      while (minor_dragged_item_extent.max
             >= minor_item_after_extent.min + (item_minor_dimension / 2)) {
        minor_item_after_extent.min += item_minor_dimension;
        list.available_index_ += list.wrap_count_;
      }
    }

    const extent_t major_dragged_item_extent =
      item_extents(list.order_, dragged_item_bound);
    if (const int32_t major_index_before = list.available_index_ - 1;
        major_index_before >= 0
        && major_index_before / list.wrap_count_
             == list.available_index_ / list.wrap_count_) {
      extent_t major_item_before_extent =
        item_extents(list.order_, calculate_bound(list, major_index_before));
      while (major_dragged_item_extent.min
             < major_item_before_extent.max - (item_major_dimension / 2)) {
        major_item_before_extent.max -= item_major_dimension;
        list.available_index_--;
      }
    }

    if (const int32_t major_index_after = list.available_index_ + 1;
        major_index_after < list.item_count_
        && major_index_after / list.wrap_count_
             == list.available_index_ / list.wrap_count_) {
      extent_t major_item_after_extent =
        item_extents(list.order_, calculate_bound(list, major_index_after));
      while (major_dragged_item_extent.max
             >= major_item_after_extent.min + (item_major_dimension / 2)) {
        major_item_after_extent.min += item_major_dimension;
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
      list_position + item_offset(list, index + offset), list.item_size_, item);
  }
}

void press_list(list_t& list, const as::vec2i& mouse_position) {
  for (int32_t index = 0; index < list.item_count_; index++) {
    const bound_t bound = calculate_bound(list, index);
    if (contained(bound, mouse_position)) {
      list.selected_index_ = index;
      list.available_index_ = index;
      list.drag_position_ = list.position_ + item_offset(list, index);
      break;
    }
  }
}

void move_list(list_t& list, const as::vec2i& movement_delta) {
  if (list.selected_index_ != -1) {
    list.drag_position_ += movement_delta;
  }
}
