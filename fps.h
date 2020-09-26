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

  int64_t samples[MaxSamples] = {};
  int head = 0;
  int tail = MaxSamples - 1;
  bool initialized = false;
};

int64_t calculateWindow(Fps& fps, int64_t now);

} // namespace fps
