#pragma once

#include "as/as-vec.hpp"
#include "as-camera/as-camera-controller.hpp"
#include "bec/bitfield-enum-class.hpp"

#include <memory>
#include <vector>
#include <optional>

union SDL_Event;

namespace asc
{

enum class MotionType;
struct CameraControl;
struct CameraProperties;

} // namespace asc

// Cameras
// stack
// return false to exit (except if top of stack)

// no starting camera - no transition
// both sub cameras are running

// must answer - how do they compose

// loads of little behaviors 

// look
// pan
// 

// behavior -> active() (could always return true)
// camera has a list of behaviors
// camera -> exclusive (cannot begin while other actions are running
//                      and other actions cannot begin while it is running)

class CameraInput
{
public:
  enum class Activation
  {
    Idle,
    Begin,
    End
  };

  virtual ~CameraInput() = default;

  bool didBegin() const { return activation_ == Activation::Begin; }
  bool didEnd() const { return activation_ == Activation::End; }

  virtual void handleEvents(const SDL_Event* event) = 0;
  virtual asc::Camera stepCamera(
    const asc::Camera& target_camera, const as::vec2i& mouse_delta) = 0;

protected:
  Activation activation_;
};

asc::Camera smoothCamera(
  const asc::Camera& current_camera, const asc::Camera& target_camera,
  const float dt);

class Cameras // could also be a CameraInput?
{
public:
  void handleEvents(const SDL_Event* event);
  asc::Camera stepCamera(const asc::Camera& target_camera);

  std::vector<CameraInput*> active_camera_inputs_;
  std::vector<CameraInput*> idle_camera_inputs_;

  std::optional<as::vec2i> last_mouse_position_; 
  as::vec2i current_mouse_position_;
};

class LookCameraInput : public CameraInput
{
public:
  void handleEvents(const SDL_Event* event) override;
  asc::Camera stepCamera(
    const asc::Camera& target_camera, const as::vec2i& mouse_delta) override;
};

class PanCameraInput : public CameraInput
{
public:
  void handleEvents(const SDL_Event* event) override;
  asc::Camera stepCamera(
    const asc::Camera& target_camera, const as::vec2i& mouse_delta) override;
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
