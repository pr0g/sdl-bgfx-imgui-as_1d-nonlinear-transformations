#include "fps.h"

namespace fps
{

int64_t calculateWindow(Fps& fps, const int64_t now)
{
  if (!fps.initialized_ && fps.head_ == fps.tail_) {
    fps.initialized_ = true;
  }

  fps.samples_[fps.head_] = now;
  fps.head_ = (fps.head_ + 1) % fps.MaxSamples;

  int64_t result = -1;
  if (fps.initialized_) {
    result = fps.samples_[fps.tail_] - fps.samples_[fps.head_];
    fps.tail_ = (fps.tail_ + 1) % fps.MaxSamples;
  }

  return result;
}

} // namespace fps
