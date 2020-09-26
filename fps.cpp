#include "fps.h"

namespace fps
{

int64_t calculateWindow(Fps& fps, const int64_t now)
{
  if (!fps.initialized && fps.head == fps.tail) {
    fps.initialized = true;
  }

  fps.samples[fps.head] = now;
  fps.head = (fps.head + 1) % fps.MaxSamples;

  int64_t result = -1;
  if (fps.initialized) {
    result = fps.samples[fps.tail] - fps.samples[fps.head];
    fps.tail = (fps.tail + 1) % fps.MaxSamples;
  }

  return result;
}

} // namespace fps
