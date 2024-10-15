#include "list.h"

#include <catch2/catch_test_macros.hpp>
#include <thh-bgfx-debug/debug-color.hpp>

TEST_CASE("Verify list interaction")
{
  auto items = std::vector<item_t>{
    {.color_ = dbg::encodeColorAbgr((uint8_t)255, 255, 255, 255),
     .name_ = "Item 1"},
    {.color_ = dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255),
     .name_ = "Item 2"},
    {.color_ = dbg::encodeColorAbgr((uint8_t)0, 255, 0, 255),
     .name_ = "Item 3"},
    {.color_ = dbg::encodeColorAbgr((uint8_t)0, 0, 255, 255),
     .name_ = "Item 4"},
    {.color_ = dbg::encodeColorAbgr((uint8_t)255, 255, 0, 255),
     .name_ = "Item 5"},
    {.color_ = dbg::encodeColorAbgr((uint8_t)0, 255, 255, 255),
     .name_ = "Item 6"},
  };

  list_t list;
  list.items_ = items.data();
  list.item_count_ = items.size();
  list.item_stride_ = sizeof(item_t);
  list.position_ = as::vec2i(200, 200);
  list.item_size_ = as::vec2i(200, 50);

  // const int32_t
  const int32_t vertical_offset = list.item_size_.y + list.vertical_spacing_;
  const as::vec2i half_item_size = list.item_size_ / 2;

  SECTION("List item can be dragged down")
  {
    // press center of top of list
    press_list(list, list.position_ + half_item_size);
    move_list(list, vertical_offset);
    update_list(
      list,
      [](const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // white box has been dragged down to the second position
        if (position.y == 255) {
          CHECK(
            list_item->color_
            == dbg::encodeColorAbgr((uint8_t)255, 255, 255, 255));
          CHECK(list_item->name_ == "Item 1");
        }
        // red box has been pushed up to the first position
        if (position.y == 200) {
          CHECK(
            list_item->color_ == dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255));
          CHECK(list_item->name_ == "Item 2");
        }
      });
  }

  SECTION("List item can be dragged up")
  {
    // press center of top of list
    press_list(
      list,
      list.position_ + half_item_size + as::vec2i(0, vertical_offset * 5));
    move_list(list, -vertical_offset);
    update_list(
      list,
      [&list, vertical_offset](
        const as::vec2i& position, const as::vec2i& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // cyan box has been dragged up to the fifth position
        if (position.y == list.position_.y + vertical_offset * 4) {
          CHECK(
            list_item->color_
            == dbg::encodeColorAbgr((uint8_t)0, 255, 255, 255));
          CHECK(list_item->name_ == "Item 6");
        }
        // yellow box has been pushed down to the sixth position
        if (position.y == list.position_.y + vertical_offset * 5) {
          CHECK(
            list_item->color_
            == dbg::encodeColorAbgr((uint8_t)255, 255, 0, 255));
          CHECK(list_item->name_ == "Item 5");
        }
      });
  }
}
