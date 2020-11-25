#include "as-camera-sdl.h"
#include "SDL.h"

void Cameras::handleEvents(const SDL_Event* event)
  {
    switch (event->type) {
      case SDL_MOUSEMOTION: {
        const auto* mouse_motion_event = (SDL_MouseMotionEvent*)event;
        current_mouse_position_ = as::vec2i(mouse_motion_event->x, mouse_motion_event->y);
        // handle mouse warp gracefully
        if (std::abs(current_mouse_position_.x - last_mouse_position_.value_or(current_mouse_position_).x) >= 500) {
          last_mouse_position_->x = current_mouse_position_.x;
        }
        if (std::abs(current_mouse_position_.y - last_mouse_position_.value_or(current_mouse_position_).y) >= 500) {
          last_mouse_position_->y = current_mouse_position_.y;
        }
      }
      break;
    }

    for (auto* camera_input : active_camera_inputs_) {
      camera_input->handleEvents(event);
    }

    for (auto* camera_input : idle_camera_inputs_) {
      camera_input->handleEvents(event);
    }

    for (int i = 0; i < idle_camera_inputs_.size();) {
      auto* camera_input = idle_camera_inputs_[i];
      if (camera_input->didBegin()) {
        active_camera_inputs_.push_back(camera_input);
        idle_camera_inputs_[i] = idle_camera_inputs_[idle_camera_inputs_.size() - 1];
        idle_camera_inputs_.pop_back();
      } else {
        i++;
      }
    }

    for (int i = 0; i < active_camera_inputs_.size();) {
      auto* camera_input = active_camera_inputs_[i];
      if (camera_input->didEnd()) {
        idle_camera_inputs_.push_back(camera_input);
        active_camera_inputs_[i] = active_camera_inputs_[active_camera_inputs_.size() - 1];
        active_camera_inputs_.pop_back();
      } else {
        i++;
      }
    }
  }

  asc::Camera Cameras::stepCamera(const asc::Camera& target_camera)
  {
    auto mouse_delta =
      current_mouse_position_
      - last_mouse_position_.value_or(current_mouse_position_);

    last_mouse_position_ = current_mouse_position_;

    asc::Camera next_camera = target_camera;
    // accumulate
    for (auto* camera_input : active_camera_inputs_) {
      next_camera = camera_input->stepCamera(next_camera, mouse_delta);
    }

    return next_camera;
  }

void LookCameraInput::handleEvents(const SDL_Event* event)
{
  switch (event->type) {
    case SDL_MOUSEBUTTONDOWN: {
      const auto* mouseEvent = (SDL_MouseButtonEvent*)event;
      if (mouseEvent->button == SDL_BUTTON_RIGHT) {
        activation_ = Activation::Begin;
      }
    }
    break;
    case SDL_MOUSEBUTTONUP: {
      const auto* mouseEvent = (SDL_MouseButtonEvent*)event;
      if (mouseEvent->button == SDL_BUTTON_RIGHT) {
        // last_mouse_position_.reset();
        activation_ = Activation::End;
      }
    }
    break;
    default:
      break;
  }
}

asc::Camera LookCameraInput::stepCamera(
  const asc::Camera& target_camera, const as::vec2i& mouse_delta)
{
  asc::Camera next_camera = target_camera;

  next_camera.pitch += float(mouse_delta[1]) * 0.005f;
  next_camera.yaw += float(mouse_delta[0]) * 0.005f;
  
  activation_ = Activation::Idle;

  return next_camera;
}

void PanCameraInput::handleEvents(const SDL_Event* event)
{
  switch (event->type) {
    case SDL_MOUSEBUTTONDOWN: {
      const auto* mouseEvent = (SDL_MouseButtonEvent*)event;
      if (mouseEvent->button == SDL_BUTTON_LEFT) {
        activation_ = Activation::Begin;
      }
    }
    break;
    case SDL_MOUSEBUTTONUP: {
      const auto* mouseEvent = (SDL_MouseButtonEvent*)event;
      if (mouseEvent->button == SDL_BUTTON_LEFT) {
        activation_ = Activation::End;
      }
    }
    break;
    default:
      break;
  }
}

asc::Camera PanCameraInput::stepCamera(
  const asc::Camera& target_camera, const as::vec2i& mouse_delta)
{
  asc::Camera next_camera = target_camera;

  const as::mat3 orientation = next_camera.transform().rotation;

  const auto basis_x = as::mat3_basis_x(orientation);
  const auto basis_y = as::mat3_basis_y(orientation);

  const auto delta_pan_x = float(mouse_delta.x) * basis_x * 0.01f/** props.pan_speed*/;
  const auto delta_pan_y = float(mouse_delta.y) * basis_y * 0.01f/** props.pan_speed*/;

  next_camera.look_at += delta_pan_x * /*props.pan_invert_x*/ -1.0f;
  next_camera.look_at += delta_pan_y /*props.pan_invert_y*/;

  activation_ = Activation::Idle;

  return next_camera;
}

asc::Camera smoothCamera(
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

asc::MotionType motionFromKey(int key)
{
  switch (key) {
    case SDL_SCANCODE_W:
      return asc::MotionType::Forward;
    case SDL_SCANCODE_S:
      return asc::MotionType::Backward;
    case SDL_SCANCODE_A:
      return asc::MotionType::Left;
    case SDL_SCANCODE_D:
      return asc::MotionType::Right;
    case SDL_SCANCODE_Q:
      return asc::MotionType::Down;
    case SDL_SCANCODE_E:
      return asc::MotionType::Up;
    case SDL_SCANCODE_UP:
      return asc::MotionType::ScrollIn;
    case SDL_SCANCODE_DOWN:
      return asc::MotionType::ScrollOut;
    case SDL_SCANCODE_R:
      return asc::MotionType::ScrollReset;
    case SDL_SCANCODE_RIGHT:
      return asc::MotionType::PushOut;
    case SDL_SCANCODE_LEFT:
      return asc::MotionType::PullIn;
    default:
      return asc::MotionType::None;
  }
}

void updateCameraControlKeyboardSdl(
  const SDL_Event& event, asc::CameraControl& control,
  const asc::CameraProperties& props)
{
  using bec::operator^=;
  using bec::operator|=;

  switch (event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
      const int key = event.key.keysym.scancode;
      if (event.type == SDL_KEYDOWN) {
        control.motion |= motionFromKey(key);
        if (key == SDL_SCANCODE_LALT) {
          if (control.mode == asc::Mode::None) {
            control.mode = asc::Mode::Orbit;
          }
        }
      }
      if (event.type == SDL_KEYUP) {
        control.motion ^= motionFromKey(key);
        if (key == SDL_SCANCODE_LALT) {
          if (control.mode == asc::Mode::Orbit) {
            control.mode = asc::Mode::None;
          }
        }
      }
    } break;
    default:
      break;
  }
}

MouseState mouseState()
{
  using bec::operator|=;

  MouseState mouse_state{};
  const int buttons =
    SDL_GetGlobalMouseState(&mouse_state.xy[0], &mouse_state.xy[1]);
  mouse_state.buttons |= ((buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0)
                         ? MouseButtons::Lmb
                         : MouseButtons::None;
  mouse_state.buttons |= ((buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0)
                         ? MouseButtons::Rmb
                         : MouseButtons::None;
  mouse_state.buttons |= ((buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0)
                         ? MouseButtons::Mmb
                         : MouseButtons::None;
  return mouse_state;
}

void updateCameraControlMouseSdl(
  asc::CameraControl& control, const asc::CameraProperties& props,
  MouseState& prev_mouse_state)
{
  const MouseState mouse_state = mouseState();

  using bec::operator&;
  if ((mouse_state.buttons & MouseButtons::Rmb) == MouseButtons::Rmb) {
    if (control.mode == asc::Mode::None) {
      control.mode = asc::Mode::Look;
    }
  } else {
    if (control.mode == asc::Mode::Look) {
      control.mode = asc::Mode::None;
    }
  }

  const as::vec2i delta = mouse_state.xy - prev_mouse_state.xy;

  if (
    (control.mode == asc::Mode::Orbit
     && ((mouse_state.buttons & MouseButtons::Lmb) == MouseButtons::Lmb))
    || control.mode == asc::Mode::Look) {
    control.pitch += float(delta[1]) * props.rotate_speed;
    control.yaw += float(delta[0]) * props.rotate_speed;
  }

  if (
    control.mode == asc::Mode::Orbit
    && ((mouse_state.buttons & MouseButtons::Rmb) == MouseButtons::Rmb)) {
    control.dolly_delta = as::vec2i(0, delta.y);
    control.pan_delta = as::vec2i::zero();
  } else if ((mouse_state.buttons & MouseButtons::Mmb) == MouseButtons::Mmb) {
    control.pan_delta = delta;
    control.dolly_delta = as::vec2i::zero();
  } else {
    control.pan_delta = as::vec2i::zero();
    control.dolly_delta = as::vec2i::zero();
  }

  prev_mouse_state = mouse_state;
}
