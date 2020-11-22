#include "as-camera-sdl.h"
#include "SDL.h"

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
        last_mouse_position_.reset();
        activation_ = Activation::End;
      }
    }
    break;
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
    default:
      break;
  }
}

asc::Camera LookCameraInput::stepCamera(const asc::Camera& target_camera)
{
  asc::Camera next_camera = target_camera;

  auto mouse_delta =
    current_mouse_position_
    - last_mouse_position_.value_or(current_mouse_position_);

  last_mouse_position_ = current_mouse_position_;

  next_camera.pitch += float(mouse_delta[1]) * 0.005f;
  next_camera.yaw += float(mouse_delta[0]) * 0.005f;
  
  activation_ = Activation::Idle;

  return next_camera;
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
