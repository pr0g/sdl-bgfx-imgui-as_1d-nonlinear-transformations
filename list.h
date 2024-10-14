#pragma once

#include <as/as-math-ops.hpp>

#include <vector>

struct item_t
{
  uint32_t color_;
};

struct list_t
{
  as::vec2 position_;
  as::vec2 item_size_;
  float spacing_ = 5.0f;
  std::vector<item_t> items_;
};
