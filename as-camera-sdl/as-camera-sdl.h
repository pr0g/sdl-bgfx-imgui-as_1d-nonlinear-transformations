#pragma once

#include "as/as-vec.hpp"
#include "as-camera/as-camera-controller.hpp"
#include "bec/bitfield-enum-class.hpp"

union SDL_Event;

namespace asc
{

enum class MotionType;
struct CameraControl;
struct CameraProperties;

} // namespace asc

class CameraInputSDL : asc::CameraInput
{
public:
  enum class Input
  {
    None        = 0,
    EnterOrbit  = 1 << 0,
    LeaveOrbit  = 1 << 1,
    EnterLook   = 1 << 2,
    LeaveLook   = 1 << 3,
    // ForwardMotion
    // BackwardMotion
    // etc..
  };

  void handleEvents(const SDL_Event* event);

  void stepCamera() override;
  asc::Camera nextCamera() const override;
  // float pitchDelta() const override;
  // float yawDelta() const override;
  // as::vec3 lookAtDelta() const override;
private:
  enum class Mode
  {
    None,
    Orbit,
    Look
  };

  Input input_;

  Mode mode_;

  as::vec2i last_mouse_position_; 
  as::vec2i mouse_delta_; // look_delta - 
  // as::vec2i translation_delta; ?
};

template<>
struct bec::EnableBitMaskOperators<CameraInputSDL::Input>
{
  static const bool Enable = true;
};


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
