#include "list.h"
#include "scenes/list-scene.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Bound calculation") {
  list_t list;
  list.spacing_ = 5;
  list.position_ = as::vec2i(200, 200);
  list.item_size_ = as::vec2i(200, 50);
  list.direction_ = direction_e::vertical;
  list.wrap_count_ = 0;

  const bound_t second_item_bound = calculate_bound(list, 1);
  CHECK(second_item_bound.top_left_.x == 200);
  CHECK(second_item_bound.top_left_.y == 255);
  CHECK(second_item_bound.bottom_right_.x == 400);
  CHECK(second_item_bound.bottom_right_.y == 305);

  const bound_t fourth_item_bound = calculate_bound(list, 3);
  CHECK(fourth_item_bound.top_left_.x == 200);
  CHECK(fourth_item_bound.top_left_.y == 365);
  CHECK(fourth_item_bound.bottom_right_.x == 400);
  CHECK(fourth_item_bound.bottom_right_.y == 415);
}

TEST_CASE("List interaction vertical") {
  auto list_items = std::vector<item_t>{
    {.color_ = white, .name_ = "Item 1"},  {.color_ = red, .name_ = "Item 2"},
    {.color_ = green, .name_ = "Item 3"},  {.color_ = blue, .name_ = "Item 4"},
    {.color_ = yellow, .name_ = "Item 5"}, {.color_ = cyan, .name_ = "Item 6"},
  };

  list_t list;
  list.items_ = list_items.data();
  list.item_count_ = list_items.size();
  list.item_stride_ = sizeof(item_t);
  list.position_ = as::vec2i(200, 200);
  list.item_size_ = as::vec2i(200, 50);
  list.direction_ = direction_e::vertical;
  list.wrap_count_ = 0;

  const int32_t vertical_offset = list.item_size_.y + list.spacing_;
  const as::vec2i half_item_size = list.item_size_ / 2;

  SECTION("List item can be dragged down by one item") {
    // press center of first list item
    press_list(list, list.position_ + half_item_size);
    move_list(list, as::vec2i::axis_y(vertical_offset));
    update_list(
      list,
      [](const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // white box has been dragged down to the second position
        if (position.y == 255) {
          CHECK(list_item->color_ == white);
          CHECK(list_item->name_ == "Item 1");
        }
        // red box has been pushed up to the first position
        if (position.y == 200) {
          CHECK(list_item->color_ == red);
          CHECK(list_item->name_ == "Item 2");
        }
      });
  }

  SECTION("List item can be dragged up by one item") {
    // press center of last list item
    press_list(
      list,
      list.position_ + half_item_size + as::vec2i::axis_y(vertical_offset * 5));
    move_list(list, as::vec2i::axis_y(-vertical_offset));
    update_list(
      list,
      [&list, vertical_offset](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // cyan box has been dragged up to the fifth position
        if (position.y == list.position_.y + vertical_offset * 4) {
          CHECK(list_item->color_ == cyan);
          CHECK(list_item->name_ == "Item 6");
        }
        // yellow box has been pushed down to the sixth position
        if (position.y == list.position_.y + vertical_offset * 5) {
          CHECK(list_item->color_ == yellow);
          CHECK(list_item->name_ == "Item 5");
        }
      });
  }

  SECTION("List item can be dragged down by multiple items") {
    const auto press_start =
      list.position_ + half_item_size + as::vec2i::axis_y(vertical_offset * 1);
    const auto press_delta = vertical_offset * 3;
    // press center of second list item
    press_list(list, press_start);
    // move down to fourth item
    move_list(list, as::vec2i::axis_y(press_delta));
    update_list(
      list,
      [&list, vertical_offset, press_start, press_delta, half_item_size](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // green box has been pushed up to second position
        if (position.y == list.position_.y + vertical_offset * 1) {
          CHECK(list_item->color_ == green);
          CHECK(list_item->name_ == "Item 3");
        }
        // red box has been pushed down to fifth position
        if (position.y == press_start.y - half_item_size.y + press_delta) {
          CHECK(list_item->color_ == red);
          CHECK(list_item->name_ == "Item 2");
        }
      });
  }

  SECTION("List item can be dragged up by multiple items") {
    const auto press_start =
      list.position_ + half_item_size + as::vec2i::axis_y(vertical_offset * 5);
    const auto press_delta = -vertical_offset * 4;
    // press center of last list item
    press_list(list, press_start);
    // move up to second item
    move_list(list, as::vec2i::axis_y(press_delta));
    update_list(
      list,
      [&list, vertical_offset, press_start, press_delta, half_item_size](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // cyan box has been moved up to second position
        if (position.y == press_start.y - half_item_size.y + press_delta) {
          CHECK(list_item->color_ == cyan);
          CHECK(list_item->name_ == "Item 6");
        }
        // yellow box has been pushed down to sixth position
        if (position.y == list.position_.y + vertical_offset * 5) {
          CHECK(list_item->color_ == yellow);
          CHECK(list_item->name_ == "Item 5");
        }
      });
  }

  SECTION("List order updated after release") {
    // press third item
    const auto press_start =
      list.position_ + half_item_size + as::vec2i(0, vertical_offset * 2);
    // drag down by two
    const auto press_delta = vertical_offset * 2;
    press_list(list, press_start);
    move_list(list, as::vec2i::axis_y(press_delta));
    update_list(list, [](const as::vec2i&, const as::vec2i&, const void*) {});
    release_list<item_t>(list);

    auto expected_items = std::vector<item_t>{
      {.color_ = white, .name_ = "Item 1"},
      {.color_ = red, .name_ = "Item 2"},
      {.color_ = blue, .name_ = "Item 4"},
      {.color_ = yellow, .name_ = "Item 5"},
      {.color_ = green, .name_ = "Item 3"},
      {.color_ = cyan, .name_ = "Item 6"},
    };

    const auto items = static_cast<std::byte*>(list.items_);
    for (int32_t index = 0; index < list.item_count_; index++) {
      const void* opaque_item = items + index * list.item_stride_;
      const auto* item = static_cast<const item_t*>(opaque_item);
      CHECK(item->name_ == expected_items[index].name_);
      CHECK(item->color_ == expected_items[index].color_);
    }
  }
}

TEST_CASE("List interaction horizontal") {
  auto list_items = std::vector<item_t>{
    {.color_ = white, .name_ = "Item 1"},  {.color_ = red, .name_ = "Item 2"},
    {.color_ = green, .name_ = "Item 3"},  {.color_ = blue, .name_ = "Item 4"},
    {.color_ = yellow, .name_ = "Item 5"}, {.color_ = cyan, .name_ = "Item 6"},
  };

  list_t list;
  list.items_ = list_items.data();
  list.item_count_ = list_items.size();
  list.item_stride_ = sizeof(item_t);
  list.position_ = as::vec2i(600, 300);
  list.item_size_ = as::vec2i(50, 100);
  list.direction_ = direction_e::horizontal;
  list.wrap_count_ = 0;

  const int32_t horizontal_offset = list.item_size_.x + list.spacing_;
  const as::vec2i half_item_size = list.item_size_ / 2;

  SECTION("List item can be dragged right by one item") {
    // press center of first list item
    press_list(list, list.position_ + half_item_size);
    // move right be one
    move_list(list, as::vec2i::axis_x(horizontal_offset));
    update_list(
      list,
      [](const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // white box has been dragged down to the second position
        if (position.x == 655) {
          CHECK(list_item->color_ == white);
          CHECK(list_item->name_ == "Item 1");
        }
        // red box has been pushed up to the first position
        if (position.x == 600) {
          CHECK(list_item->color_ == red);
          CHECK(list_item->name_ == "Item 2");
        }
      });
  }

  SECTION("List item can be dragged left by one item") {
    // press center of last list item
    press_list(
      list, list.position_ + half_item_size
              + as::vec2i::axis_x(horizontal_offset * 5));
    move_list(list, as::vec2i::axis_x(-horizontal_offset));
    update_list(
      list,
      [&list, horizontal_offset](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // cyan box has been dragged up to the fifth position
        if (position.x == list.position_.x + horizontal_offset * 4) {
          CHECK(list_item->color_ == cyan);
          CHECK(list_item->name_ == "Item 6");
        }
        // yellow box has been pushed down to the sixth position
        if (position.x == list.position_.x + horizontal_offset * 5) {
          CHECK(list_item->color_ == yellow);
          CHECK(list_item->name_ == "Item 5");
        }
      });
  }

  SECTION("List item can be dragged right by multiple items") {
    const auto press_start = list.position_ + half_item_size
                           + as::vec2i::axis_x(horizontal_offset * 1);
    const auto press_delta = horizontal_offset * 3;
    // press center of second list item
    press_list(list, press_start);
    // move down to fourth item
    move_list(list, as::vec2i::axis_x(press_delta));
    update_list(
      list,
      [&list, horizontal_offset, press_start, press_delta, half_item_size](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // green box has been pushed left to second position
        if (position.x == list.position_.x + horizontal_offset * 1) {
          CHECK(list_item->color_ == green);
          CHECK(list_item->name_ == "Item 3");
        }
        // red box has been pushed right to fifth position
        if (position.x == press_start.x - half_item_size.x + press_delta) {
          CHECK(list_item->color_ == red);
          CHECK(list_item->name_ == "Item 2");
        }
      });
  }

  SECTION("List item can be dragged left by multiple items") {
    const auto press_start = list.position_ + half_item_size
                           + as::vec2i::axis_x(horizontal_offset * 5);
    const auto press_delta = -horizontal_offset * 4;
    // press center of last list item
    press_list(list, press_start);
    // move up to second item
    move_list(list, as::vec2i::axis_x(press_delta));
    update_list(
      list,
      [&list, horizontal_offset, press_start, press_delta, half_item_size](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // cyan box has been moved left to second position
        if (position.x == press_start.x - half_item_size.x + press_delta) {
          CHECK(list_item->color_ == cyan);
          CHECK(list_item->name_ == "Item 6");
        }
        // yellow box has been pushed right to sixth position
        if (position.x == list.position_.x + horizontal_offset * 5) {
          CHECK(list_item->color_ == yellow);
          CHECK(list_item->name_ == "Item 5");
        }
      });
  }
}
