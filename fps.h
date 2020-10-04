#pragma once

#include <cstdint>

namespace fps
{

struct Fps
{
  enum
  {
    MaxSamples = 20
  };

  int64_t samples_[MaxSamples] = {};
  int head_ = 0;
  int tail_ = MaxSamples - 1;
  bool initialized_ = false;
};

int64_t calculateWindow(Fps& fps, int64_t now);

} // namespace fps
