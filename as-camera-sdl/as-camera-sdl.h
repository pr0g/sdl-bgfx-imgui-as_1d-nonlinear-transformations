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
  virtual asc::Camera stepCamera(const asc::Camera& target_camera) = 0;

protected:
  Activation activation_;
};

inline asc::Camera smoothCamera(
  const asc::Camera& current_camera, const asc::Camera& target_camera,
  const float dt)
{
  auto clamp_rotation = [](const float angle) {
    return std::fmod(angle + as::k_tau, as::k_tau);
  };

  // keep yaw in 0 - 360 range
  float target_yaw = clamp_rotation(target_camera.yaw);
  const float current_yaw = clamp_rotation(current_camera.yaw);

  // ensure smooth transition when moving across 0 - 360 boundary
  const float yaw_delta = target_yaw - current_yaw;
  if (std::abs(yaw_delta) >= as::k_pi) {
    target_yaw -= as::k_tau * as::sign(yaw_delta);
  }

  // clamp pitch to be +-90 degrees
  const float target_pitch =
    as::clamp(target_camera.pitch, -as::k_pi * 0.5f, as::k_pi * 0.5f);

  asc::Camera camera;
  // https://www.gamasutra.com/blogs/ScottLembcke/20180404/316046/Improved_Lerp_Smoothing.php
  const float look_rate = exp2(/*props.look_smoothness*/5.0f);
  const float look_t = exp2(-look_rate * dt);
  camera.pitch = as::mix(target_pitch, current_camera.pitch, look_t);
  camera.yaw = as::mix(target_yaw, current_yaw, look_t);
  const float move_rate = exp2(/*props.move_smoothness*/5.0f);
  const float move_t = exp2(-move_rate * dt);
  camera.focal_dist = as::mix(target_camera.focal_dist, current_camera.focal_dist, move_t);
  camera.look_at = as::vec_mix(target_camera.look_at, current_camera.look_at, move_t);
  return camera;
}

class Cameras // could also be a CameraInput?
{
public:
  void handleEvents(const SDL_Event* event)
  {
    for (auto* camera : cameras_) {
      camera->handleEvents(event);
    }

    for (auto* camera : cameras_) {
      if (camera->didBegin() && active_camera_ == nullptr) {
        active_camera_ = camera;
        break;
      }
    }

    for (const auto* camera : cameras_) {
      if (camera->didEnd() && active_camera_ != nullptr) {
        active_camera_ = nullptr;
        break;
      }
    }
  }

  asc::Camera stepCamera(const asc::Camera& target_camera)
  {
    if (active_camera_) {
      return active_camera_->stepCamera(target_camera);
    }

    return target_camera;
  }

  CameraInput* active_camera_ = nullptr;
  std::vector<CameraInput*> cameras_;
};

class LookCameraInput : public CameraInput
{
public:
  void handleEvents(const SDL_Event* event) override;
  asc::Camera stepCamera(const asc::Camera& target_camera) override;

  std::optional<as::vec2i> last_mouse_position_; 
  as::vec2i current_mouse_position_;
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
