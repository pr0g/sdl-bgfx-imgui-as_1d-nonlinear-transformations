#include "list.h"
#include "scenes/list-scene.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Bound calculation") {
  list_t list;
  list.spacing_ = 5;
  list.position_ = as::vec2i(200, 200);
  list.item_size_ = as::vec2i(200, 50);
  list.order_ = order_e::top_to_bottom;
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
  list.order_ = order_e::top_to_bottom;
  list.wrap_count_ = 0;

  list_display_t list_display;
  init_list(list, list_display);

  const int32_t vertical_offset = list.item_size_.y + list.spacing_;
  const as::vec2i half_item_size = list.item_size_ / 2;
  const float delta_time = 1.0f;

  SECTION("List item can be dragged down by one item") {
    // press center of first list item
    press_list(list, list.position_ + half_item_size);
    move_list(list, as::vec2i::axis_y(vertical_offset));
    update_list(
      list, list_display,
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
      },
      delta_time);
  }

  SECTION("List item can be dragged up by one item") {
    // press center of last list item
    press_list(
      list,
      list.position_ + half_item_size + as::vec2i::axis_y(vertical_offset * 5));
    move_list(list, as::vec2i::axis_y(-vertical_offset));
    update_list(
      list, list_display,
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
      },
      delta_time);
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
      list, list_display,
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
      },
      delta_time);
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
      list, list_display,
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
      },
      delta_time);
  }

  SECTION("List order updated after release") {
    // press third item
    const auto press_start =
      list.position_ + half_item_size + as::vec2i(0, vertical_offset * 2);
    // drag down by two
    const auto press_delta = vertical_offset * 2;
    press_list(list, press_start);
    move_list(list, as::vec2i::axis_y(press_delta));
    update_list(
      list, list_display,
      [](const as::vec2i&, const as::vec2i&, const void*) {}, delta_time);
    release_list<item_t>(list, list_display);

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
  list.order_ = order_e::left_to_right;
  list.wrap_count_ = 0;

  list_display_t list_display;
  init_list(list, list_display);

  const int32_t horizontal_offset = list.item_size_.x + list.spacing_;
  const as::vec2i half_item_size = list.item_size_ / 2;
  const float delta_time = 1.0f;

  SECTION("List item can be dragged right by one item") {
    // press center of first list item
    press_list(list, list.position_ + half_item_size);
    // move right be one
    move_list(list, as::vec2i::axis_x(horizontal_offset));
    update_list(
      list, list_display,
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
      },
      delta_time);
  }

  SECTION("List item can be dragged left by one item") {
    // press center of last list item
    press_list(
      list, list.position_ + half_item_size
              + as::vec2i::axis_x(horizontal_offset * 5));
    move_list(list, as::vec2i::axis_x(-horizontal_offset));
    update_list(
      list, list_display,
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
      },
      delta_time);
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
      list, list_display,
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
      },
      delta_time);
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
      list, list_display,
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
      },
      delta_time);
  }
}

TEST_CASE("List interaction horizontal wrapping") {
  auto list_items = std::vector<item_t>{
    {.color_ = white, .name_ = "Item 1"},
    {.color_ = red, .name_ = "Item 2"},
    {.color_ = green, .name_ = "Item 3"},
    {.color_ = blue, .name_ = "Item 4"},
    {.color_ = yellow, .name_ = "Item 5"},
    {.color_ = cyan, .name_ = "Item 6"},
    {.color_ = light_coral, .name_ = "Item 7"},
    {.color_ = pale_violet_red, .name_ = "Item 8"},
    {.color_ = tomato, .name_ = "Item 9"},
    {.color_ = orange, .name_ = "Item 10"},
    {.color_ = khaki, .name_ = "Item 11"},
    {.color_ = pale_green, .name_ = "Item 12"},
  };

  list_t list;
  list.items_ = list_items.data();
  list.item_count_ = list_items.size();
  list.item_stride_ = sizeof(item_t);
  list.position_ = as::vec2i(100, 100);
  list.item_size_ = as::vec2i(20, 50);
  list.order_ = order_e::left_to_right;
  list.wrap_count_ = 3;

  list_display_t list_display;
  init_list(list, list_display);

  const int32_t vertical_offset = list.item_size_.y + list.spacing_;
  const int32_t horizontal_offset = list.item_size_.x + list.spacing_;
  const as::vec2i half_item_size = list.item_size_ / 2;
  const float delta_time = 1.0f;

  SECTION("List item can be dragged right and down") {
    // item 2
    const auto press_start = list.position_ + half_item_size
                           + as::vec2i::axis_x(horizontal_offset * 1);
    const auto press_delta =
      as::vec2i(horizontal_offset * 1, vertical_offset * 2);
    press_list(list, press_start);
    move_list(list, press_delta);
    update_list(
      list, list_display,
      [&list, horizontal_offset, press_start, press_delta, half_item_size](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        if (position == press_start - half_item_size + press_delta) {
          CHECK(list_item->color_ == red);
          CHECK(list_item->name_ == "Item 2");
        }
        if (position == press_start - half_item_size) {
          CHECK(list_item->color_ == green);
          CHECK(list_item->name_ == "Item 3");
        }
      },
      delta_time);
  }

  SECTION("List item can be dragged left and up") {
    // item 11
    const auto press_start =
      list.position_ + half_item_size
      + as::vec2i(horizontal_offset * 1, vertical_offset * 3);
    const auto press_delta =
      as::vec2i(-horizontal_offset * 1, -vertical_offset * 3);
    press_list(list, press_start);
    move_list(list, press_delta);
    update_list(
      list, list_display,
      [&list, horizontal_offset, press_start, press_delta, half_item_size](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        if (position == press_start - half_item_size + press_delta) {
          CHECK(list_item->color_ == khaki);
          CHECK(list_item->name_ == "Item 11");
        }
        if (position == press_start - half_item_size) {
          CHECK(list_item->color_ == orange);
          CHECK(list_item->name_ == "Item 10");
        }
      },
      delta_time);
  }
}

TEST_CASE("List interaction vertical wrapping") {
  auto list_items = std::vector<item_t>{
    {.color_ = white, .name_ = "Item 1"},
    {.color_ = red, .name_ = "Item 2"},
    {.color_ = green, .name_ = "Item 3"},
    {.color_ = blue, .name_ = "Item 4"},
    {.color_ = yellow, .name_ = "Item 5"},
    {.color_ = cyan, .name_ = "Item 6"},
    {.color_ = light_coral, .name_ = "Item 7"},
    {.color_ = pale_violet_red, .name_ = "Item 8"},
    {.color_ = tomato, .name_ = "Item 9"},
    {.color_ = orange, .name_ = "Item 10"},
    {.color_ = khaki, .name_ = "Item 11"},
    {.color_ = pale_green, .name_ = "Item 12"},
  };

  list_t list;
  list.items_ = list_items.data();
  list.item_count_ = list_items.size();
  list.item_stride_ = sizeof(item_t);
  list.position_ = as::vec2i(100, 100);
  list.item_size_ = as::vec2i(20, 50);
  list.order_ = order_e::top_to_bottom;
  list.wrap_count_ = 4;

  list_display_t list_display;
  init_list(list, list_display);

  const int32_t vertical_offset = list.item_size_.y + list.spacing_;
  const int32_t horizontal_offset = list.item_size_.x + list.spacing_;
  const as::vec2i half_item_size = list.item_size_ / 2;
  const float delta_time = 1.0f;

  SECTION("List item can be dragged left and down") {
    // item 5
    const auto press_start = list.position_ + half_item_size
                           + as::vec2i::axis_x(horizontal_offset * 1);
    const auto press_delta =
      as::vec2i(-horizontal_offset * 1, vertical_offset * 3);
    press_list(list, press_start);
    move_list(list, press_delta);
    update_list(
      list, list_display,
      [&list, horizontal_offset, press_start, press_delta, half_item_size](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        if (position == press_start - half_item_size + press_delta) {
          CHECK(list_item->color_ == yellow);
          CHECK(list_item->name_ == "Item 5");
        }
        if (position == press_start - half_item_size) {
          CHECK(list_item->color_ == blue);
          CHECK(list_item->name_ == "Item 4");
        }
      },
      delta_time);
  }

  SECTION("List item can be dragged right and up") {
    // item 4
    const auto press_start =
      list.position_ + half_item_size + as::vec2i::axis_y(vertical_offset * 3);
    const auto press_delta =
      as::vec2i(horizontal_offset * 2, -vertical_offset * 3);
    press_list(list, press_start);
    move_list(list, press_delta);
    update_list(
      list, list_display,
      [&list, horizontal_offset, press_start, press_delta, half_item_size](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        if (position == press_start - half_item_size + press_delta) {
          CHECK(list_item->color_ == blue);
          CHECK(list_item->name_ == "Item 4");
        }
        if (position == press_start - half_item_size) {
          CHECK(list_item->color_ == yellow);
          CHECK(list_item->name_ == "Item 5");
        }
      },
      delta_time);
  }
}
