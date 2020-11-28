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

  void beginActivation() { activation_ = Activation::Begin; }
  void endActivation() { activation_ = Activation::End; }
  void clearActivation() { activation_ = Activation::Idle; }

  virtual void handleEvents(const SDL_Event* event) = 0;
  virtual asc::Camera stepCamera(
    const asc::Camera& target_camera, const as::vec2i& mouse_delta,
    float delta_time) = 0;
  virtual bool exclusive() const { return false; }

private:
  Activation activation_;
};

asc::Camera smoothCamera(
  const asc::Camera& current_camera, const asc::Camera& target_camera,
  const float dt);

class Cameras // could also be a CameraInput?
{
public:
  void handleEvents(const SDL_Event* event);
  asc::Camera stepCamera(const asc::Camera& target_camera,
  float delta_time);

  std::vector<CameraInput*> active_camera_inputs_;
  std::vector<CameraInput*> idle_camera_inputs_;

  std::optional<as::vec2i> last_mouse_position_; 
  as::vec2i current_mouse_position_;
};

class /*Free*/LookCameraInput : public CameraInput
{
public:
  void handleEvents(const SDL_Event* event) override;
  asc::Camera stepCamera(
    const asc::Camera& target_camera, const as::vec2i& mouse_delta,
    float delta_time) override;
};

class PanCameraInput : public CameraInput
{
public:
  void handleEvents(const SDL_Event* event) override;
  asc::Camera stepCamera(
    const asc::Camera& target_camera, const as::vec2i& mouse_delta,
    float delta_time) override;
};

class TranslateCameraInput : public CameraInput
{
public:
  void handleEvents(const SDL_Event* event) override;
  asc::Camera stepCamera(
    const asc::Camera& target_camera, const as::vec2i& mouse_delta,
    float delta_time) override;
private:
  enum class TranslationType
  {
    // clang-format off
    None        = 0,
    Forward     = 1 << 0,
    Backward    = 1 << 1,
    Left        = 1 << 2,
    Right       = 1 << 3,
    Up          = 1 << 4,
    Down        = 1 << 5,
    // clang-format on
  };

  static TranslationType translationFromKey(int key);

  TranslationType translation_ = TranslationType::None;
};

template<>
struct bec::EnableBitMaskOperators<TranslateCameraInput::TranslationType>
{
  static const bool Enable = true;
};

class OrbitLookCameraInput : public CameraInput
{
public:
  void handleEvents(const SDL_Event* event) override;
  asc::Camera stepCamera(
    const asc::Camera& target_camera, const as::vec2i& mouse_delta,
    float delta_time) override;
  bool exclusive() const override { return true; }
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
