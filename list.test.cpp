#include "list.h"

#include <catch2/catch_test_macros.hpp>
#include <thh-bgfx-debug/debug-color.hpp>

TEST_CASE("Verify list interaction")
{
  list_t list;
  list.position_ = as::vec2(200, 200);
  list.item_size_ = as::vec2(200, 50);
  list.items_ = std::vector<item_t>{
    {dbg::encodeColorAbgr((uint8_t)255, 255, 255, 255)},
    {dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255)},
    {dbg::encodeColorAbgr((uint8_t)0, 255, 0, 255)},
    {dbg::encodeColorAbgr((uint8_t)0, 0, 255, 255)},
    {dbg::encodeColorAbgr((uint8_t)255, 255, 0, 255)},
    {dbg::encodeColorAbgr((uint8_t)0, 255, 255, 255)},
  };

  SECTION("List item can be dragged down")
  {
    // press center of top of list
    press_list(list, as::vec2i(300, 225));
    move_list(list, 200.0f);
    update_list(
      list,
      [](const as::vec2& position, const as::vec2& size, const uint32_t color) {
        // white box has been dragged down to the second position
        if (as::real_near(position.y, 405.0f)) {
          CHECK(color == dbg::encodeColorAbgr((uint8_t)255, 255, 255, 255));
        }
        // red box has been pushed up to the first position
        if (as::real_near(position.y, 200.0f)) {
          CHECK(color == dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255));
        }
      });
  }
}
