#pragma once

#include "as/as-vec.hpp"
#include "bec/bitfield-enum-class.hpp"

union SDL_Event;

namespace asc
{

enum class MotionType;
struct CameraControl;
struct CameraProperties;

} // namespace asc

enum class MouseButtons : uint8_t
{
  None = 0,
  Lmb = 1 << 0,
  Rmb = 1 << 1,
  Mmb = 1 << 2
};

template<>
struct bec::EnableBitMaskOperators<MouseButtons>
{
  static const bool Enable = true;
};

struct MouseState
{
  as::vec2i xy;
  MouseButtons buttons;
};

MouseState mouseState();
asc::MotionType motionFromKey(int key);
void updateCameraControlKeyboardSdl(
  const SDL_Event& event, asc::CameraControl& control,
  const asc::CameraProperties& props);
void updateCameraControlMouseSdl(
  asc::CameraControl& control, const asc::CameraProperties& props,
  MouseState& prev_mouse_state);
