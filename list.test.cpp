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
  list.position_ = as::vec2(200, 200);
  list.item_size_ = as::vec2(200, 50);

  SECTION("List item can be dragged down")
  {
    // press center of top of list
    press_list(list, as::vec2i(300, 225));
    move_list(list, 55.0f);
    update_list(
      list,
      [](const as::vec2& position, const as::vec2& size, const void* item) {
        const auto* list_item = static_cast<const item_t*>(item);
        // white box has been dragged down to the second position
        if (as::real_near(position.y, 255.0f)) {
          CHECK(
            list_item->color_
            == dbg::encodeColorAbgr((uint8_t)255, 255, 255, 255));
          CHECK(list_item->name_ == "Item 1");
        }
        // red box has been pushed up to the first position
        if (as::real_near(position.y, 200.0f)) {
          CHECK(
            list_item->color_ == dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255));
          CHECK(list_item->name_ == "Item 2");
        }
      });
  }
}
