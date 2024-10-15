#include "list.h"

// needed for debug_draw_t
#include "scenes/scene.h"

#include <as/as-math-ops.hpp>
#include <as/as-view.hpp>
#include <easy_iterator.h>

static bool contained(const bound_t& bound, const as::vec2& point)
{
  if (
    point.x >= bound.top_left_.x && point.x < bound.bottom_right_.x
    && point.y >= bound.top_left_.y && point.y < bound.bottom_right_.y) {
    return true;
  }
  return false;
}

void update_list(list_t& list, const draw_box_fn& draw_box)
{
  const auto item_size = list.item_size_;
  const auto list_position = list.position_;

  auto items = static_cast<std::byte*>(list.items_);
  if (list.selected_index_ != -1) {
    void* item = items + list.selected_index_ * list.item_stride_;
    draw_box(list.drag_position_, list.item_size_, item);
    const bound_t item_bound = bound_t{
      .top_left_ = as::vec2(list_position.x, list.drag_position_.y),
      .bottom_right_ = as::vec2(
        list_position.x + item_size.x, list.drag_position_.y + item_size.y)};

    if (const int index_before = list.available_index_ - 1; index_before >= 0) {
      const auto position =
        list.position_
        + as::vec2(0.0f, (item_size.y + list.vertical_spacing_) * index_before);
      const bound_t item_before_bound = bound_t{
        as::vec2(
          list_position.x,
          list_position.y
            + index_before * (item_size.y + list.vertical_spacing_)),
        as::vec2(
          list_position.x + item_size.x,
          list_position.y + item_size.y
            + index_before * (item_size.y + list.vertical_spacing_))};
      if (
        item_bound.top_left_.y
        < item_before_bound.bottom_right_.y - (item_size.y / 2)) {
        list.available_index_--;
      }
    }

    if (const int index_after = list.available_index_ + 1;
        index_after < list.item_count_) {
      const auto position =
        list.position_
        + as::vec2(0.0f, (item_size.y + list.vertical_spacing_) * index_after);
      const bound_t item_after_bound = bound_t{
        as::vec2(
          list_position.x,
          list_position.y
            + index_after * (item_size.y + list.vertical_spacing_)),
        as::vec2(
          list_position.x + item_size.x,
          list_position.y + item_size.y
            + index_after * (item_size.y + list.vertical_spacing_))};
      if (
        item_bound.bottom_right_.y
        >= item_after_bound.top_left_.y + (item_size.y / 2)) {
        list.available_index_++;
      }
    }
  }

  for (int index = 0; index < list.item_count_; index++) {
    void* item = items + index * list.item_stride_;
    if (index == list.selected_index_) {
      continue;
    }
    int offset = 0;
    if (index >= list.available_index_ && index < list.selected_index_) {
      offset = 1;
    }
    if (index <= list.available_index_ && index > list.selected_index_) {
      offset = -1;
    }
    draw_box(
      list_position
        + as::vec2(
          0.0f, (item_size.y + list.vertical_spacing_) * (index + offset)),
      item_size, item);
  }

  // debug bounds drawing
  // for (const auto& [index, item] : ei::enumerate(list.items_)) {
  //   const bound_t bound = bound_t{
  //     as::vec2(
  //       list_position.x,
  //       list_position.y + index * (item_size.y + list.vertical_spacing_)),
  //     as::vec2(
  //       list_position.x + item_size.x,
  //       list_position.y + item_size.y
  //         + index * (item_size.y + list.vertical_spacing_))};
  //   draw_box(debug_draw, bound, item.color_);
  // }
}

void press_list(list_t& list, const as::vec2i& mouse_position)
{
  const as::vec2 list_position = list.position_;
  const as::vec3 item_size = list.item_size_;
  for (int index = 0; index < list.item_count_; index++) {
    const bound_t bound = bound_t{
      .top_left_ = as::vec2(
        list_position.x,
        list_position.y + index * (item_size.y + list.vertical_spacing_)),
      .bottom_right_ = as::vec2(
        list_position.x + item_size.x,
        list_position.y + item_size.y
          + index * (item_size.y + list.vertical_spacing_))};
    if (contained(bound, as::vec2_from_vec2i(mouse_position))) {
      list.selected_index_ = index;
      list.available_index_ = index;
      list.drag_position_ =
        list_position
        + as::vec2(0.0f, index * (item_size.y + list.vertical_spacing_));
    }
  }
}

void release_list(list_t& list)
{
  // auto temp = std::move(list.items_[list.selected_index_]);
  // list.items_.erase(std::begin(list.items_) + list.selected_index_);
  // list.items_.insert(
  //   std::begin(list.items_) + list.available_index_, std::move(temp));
  list.selected_index_ = -1;
  list.available_index_ = -1;
  list.drag_position_ = as::vec2::zero();
}

void move_list(list_t& list, const float movement_delta)
{
  if (list.selected_index_ != -1) {
    list.drag_position_.y += movement_delta;
  }
}
