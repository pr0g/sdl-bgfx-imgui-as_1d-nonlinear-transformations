#pragma once

#include "as/as-math-ops.hpp"

#include <array>

namespace dbg
{

class CurveHandles
{
  std::array<as::vec3, 64> handles_;
  as::vec3 press_;
  as::vec3 center_;
  as::index next_handle_ = 0;
  as::index drag_handle_ = -1;
  bool drag_ = false;

public:
  static constexpr const auto HandleRadius = 0.2f;

  as::index addHandle(const as::vec3& handle)
  {
    handles_[next_handle_] = handle;
    return next_handle_++;
  }

  void tryBeginDrag(const as::vec3& hit)
  {
    press_ = hit;
    float max_dist = std::numeric_limits<float>::max();
    for (as::index index = 0; index < next_handle_; ++index) {
      const auto dist = as::vec_distance(hit, handles_[index]);
      if (dist < HandleRadius && dist < max_dist) {
        max_dist = dist;
        drag_handle_ = index;
        center_ = handles_[index];
        drag_ = true;
      }
    }
  }

  as::vec3 getHandle(const as::index index) const
  {
    if (index >= next_handle_) {
      return as::vec3::zero();
    }

    return handles_[index];
  }

  void updateDrag(const as::vec3& hit)
  {
    handles_[drag_handle_] = center_ + (hit - press_);
  }

  void clearDrag()
  {
    drag_ = false;
    drag_handle_ = -1;
  }

  bool dragging() const { return drag_; }
  as::index size() const { return next_handle_; }
};

} // namespace dbg
