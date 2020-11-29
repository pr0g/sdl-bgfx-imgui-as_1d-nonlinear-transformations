#include "as-camera-sdl.h"
#include "SDL.h"

void Cameras::handleEvents(const SDL_Event* event)
{
  switch (event->type) {
    case SDL_MOUSEMOTION: {
      const auto* mouse_motion_event = (SDL_MouseMotionEvent*)event;
      current_mouse_position_ = as::vec2i(mouse_motion_event->x, mouse_motion_event->y);
      // handle mouse warp gracefully
      if (current_mouse_position_.has_value() && last_mouse_position_.has_value()) {
        if (
          std::abs(current_mouse_position_->x - last_mouse_position_->x) >= 500) {
          last_mouse_position_->x = current_mouse_position_->x;
        }
        if (std::abs(current_mouse_position_->y - last_mouse_position_->y) >= 500) {
          last_mouse_position_->y = current_mouse_position_->y;
        }
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
}

asc::Camera Cameras::stepCamera(
  const asc::Camera& target_camera, const float delta_time)
{
  for (int i = 0; i < idle_camera_inputs_.size();) {
    auto* camera_input = idle_camera_inputs_[i];
    const bool can_begin =
      camera_input->beginning() &&
      std::all_of(active_camera_inputs_.cbegin(), active_camera_inputs_.cend(),
      [](const auto& input){ return !input->exclusive(); })
      && (!camera_input->exclusive() ||
        (camera_input->exclusive() && active_camera_inputs_.empty()));
    if (can_begin) {
      active_camera_inputs_.push_back(camera_input);
      idle_camera_inputs_[i] = idle_camera_inputs_[idle_camera_inputs_.size() - 1];
      idle_camera_inputs_.pop_back();
    } else {
      i++;
    }
  }

  auto mouse_delta =
    current_mouse_position_.has_value() && last_mouse_position_.has_value()
      ? current_mouse_position_.value() - last_mouse_position_.value()
      : as::vec2i::zero();

  if (current_mouse_position_.has_value()) {
    last_mouse_position_ = current_mouse_position_;
  }

  // accumulate
  asc::Camera next_camera = target_camera;
  for (auto* camera_input : active_camera_inputs_) {
    next_camera = camera_input->stepCamera(next_camera, mouse_delta, delta_time);
  }

  for (int i = 0; i < active_camera_inputs_.size();) {
    auto* camera_input = active_camera_inputs_[i];
    if (camera_input->ending()) {
      camera_input->clearActivation();
      idle_camera_inputs_.push_back(camera_input);
      active_camera_inputs_[i] = active_camera_inputs_[active_camera_inputs_.size() - 1];
      active_camera_inputs_.pop_back();
    } else {
      camera_input->continueActivation();
      i++;
    }
  }

  return next_camera;
}

void Cameras::reset()
{
  for (int i = 0; i < active_camera_inputs_.size();) {
    active_camera_inputs_[i]->reset();
    idle_camera_inputs_.push_back(active_camera_inputs_[i]);
    active_camera_inputs_[i] = active_camera_inputs_[active_camera_inputs_.size() - 1];
    active_camera_inputs_.pop_back();
  }
}

void LookCameraInput::handleEvents(const SDL_Event* event)
{
  switch (event->type) {
    case SDL_MOUSEBUTTONDOWN: {
      const auto* mouseEvent = (SDL_MouseButtonEvent*)event;
      if (mouseEvent->button == button_type_) {
        beginActivation();
      }
    }
    break;
    case SDL_MOUSEBUTTONUP: {
      const auto* mouseEvent = (SDL_MouseButtonEvent*)event;
      if (mouseEvent->button == button_type_) {
        endActivation();
      }
    }
    break;
    default:
      break;
  }
}

asc::Camera LookCameraInput::stepCamera(
  const asc::Camera& target_camera, const as::vec2i& mouse_delta,
  const float delta_time)
{
  asc::Camera next_camera = target_camera;

  next_camera.pitch += float(mouse_delta[1]) * 0.005f;
  next_camera.yaw += float(mouse_delta[0]) * 0.005f;

  return next_camera;
}

void PanCameraInput::handleEvents(const SDL_Event* event)
{
  switch (event->type) {
    case SDL_MOUSEBUTTONDOWN: {
      const auto* mouseEvent = (SDL_MouseButtonEvent*)event;
      if (mouseEvent->button == SDL_BUTTON_MIDDLE) {
        beginActivation();
      }
    }
    break;
    case SDL_MOUSEBUTTONUP: {
      const auto* mouseEvent = (SDL_MouseButtonEvent*)event;
      if (mouseEvent->button == SDL_BUTTON_MIDDLE) {
        endActivation();
      }
    }
    break;
    default:
      break;
  }
}

asc::Camera PanCameraInput::stepCamera(
  const asc::Camera& target_camera, const as::vec2i& mouse_delta,
  const float delta_time)
{
  asc::Camera next_camera = target_camera;

  const as::mat3 orientation = next_camera.transform().rotation;

  const auto basis_x = as::mat3_basis_x(orientation);
  const auto basis_y = as::mat3_basis_y(orientation);

  const auto delta_pan_x = float(mouse_delta.x) * basis_x * 0.01f/** props.pan_speed*/;
  const auto delta_pan_y = float(mouse_delta.y) * basis_y * 0.01f/** props.pan_speed*/;

  next_camera.look_at += delta_pan_x * /*props.pan_invert_x*/ -1.0f;
  next_camera.look_at += delta_pan_y /*props.pan_invert_y*/;

  return next_camera;
}

TranslateCameraInput::TranslationType TranslateCameraInput::translationFromKey(int key)
{
  switch (key) {
    case SDL_SCANCODE_W:
      return TranslationType::Forward;
    case SDL_SCANCODE_S:
      return TranslationType::Backward;
    case SDL_SCANCODE_A:
      return TranslationType::Left;
    case SDL_SCANCODE_D:
      return TranslationType::Right;
    case SDL_SCANCODE_Q:
      return TranslationType::Down;
    case SDL_SCANCODE_E:
      return TranslationType::Up;
    default:
      return TranslationType::None;
  }
}

void TranslateCameraInput::handleEvents(const SDL_Event* event)
{
  switch (event->type) {
    case SDL_KEYDOWN: {
      const auto* keyboardEvent = (SDL_KeyboardEvent*)event;
      using bec::operator|=;
      translation_ |= translationFromKey(keyboardEvent->keysym.scancode);
      if (translation_ != TranslationType::None) {
        beginActivation();
      }
    }
    break;
    case SDL_KEYUP: {
      const auto* keyboardEvent = (SDL_KeyboardEvent*)event;
      using bec::operator^=;
      translation_ ^= translationFromKey(keyboardEvent->keysym.scancode);
      if (translation_ == TranslationType::None) {
        endActivation();
      }
    }
    break;
    default:
      break;
  }
}

asc::Camera TranslateCameraInput::stepCamera(
    const asc::Camera& target_camera, const as::vec2i& mouse_delta, const float delta_time)
{
  asc::Camera next_camera = target_camera;

  const auto translation_basis = translationAxesFn_(next_camera);
  const auto axis_x = as::mat3_basis_x(translation_basis);
  const auto axis_y = as::mat3_basis_y(translation_basis);
  const auto axis_z = as::mat3_basis_z(translation_basis);

  using bec::operator&;

  const float speed = 10.0f;

  if ((translation_ & TranslationType::Forward) == TranslationType::Forward) {
    next_camera.look_at += axis_z * speed * /*props.translate_speed*/ delta_time;
  }

  if ((translation_ & TranslationType::Backward) == TranslationType::Backward) {
    next_camera.look_at -= axis_z * speed * /*props.translate_speed*/ delta_time;
  }

  if ((translation_ & TranslationType::Left) == TranslationType::Left) {
    next_camera.look_at -= axis_x * speed * /*props.translate_speed* */ delta_time;
  }

  if ((translation_ & TranslationType::Right) == TranslationType::Right) {
    next_camera.look_at += axis_x * speed * /*props.translate_speed* */ delta_time;
  }

  if ((translation_ & TranslationType::Up) == TranslationType::Up) {
    next_camera.look_at += axis_y * speed * /*props.translate_speed* */ delta_time;
  }

  if ((translation_ & TranslationType::Down) == TranslationType::Down) {
    next_camera.look_at -= axis_y * speed * /*props.translate_speed* */ delta_time;
  }

  if (ending()) {
    translation_ = TranslationType::None;
  }

  return next_camera;
}

void OrbitLookCameraInput::handleEvents(const SDL_Event* event)
{
  switch (event->type) {
    case SDL_KEYDOWN: {
      const auto* keyboardEvent = (SDL_KeyboardEvent*)event;
      if (keyboardEvent->keysym.scancode == SDL_SCANCODE_LALT) {
        beginActivation();
      }
    }
    break;
    case SDL_KEYUP: {
      const auto* keyboardEvent = (SDL_KeyboardEvent*)event;
      if (keyboardEvent->keysym.scancode == SDL_SCANCODE_LALT) {
        endActivation();
      }
    }
    break;
    default:
      break;
  }

  if (active()) {
    orbit_cameras_.handleEvents(event);
  }
}

static float intersectPlane(
  const as::vec3& origin, const as::vec3& direction, const as::vec4& plane)
{
  return -(as::vec_dot(origin, as::vec3_from_vec4(plane)) + plane.w)
       / as::vec_dot(direction, as::vec3_from_vec4(plane));
}

asc::Camera OrbitLookCameraInput::stepCamera(
  const asc::Camera& target_camera, const as::vec2i& mouse_delta,
  float delta_time)
{
  asc::Camera next_camera = target_camera;

  const float default_orbit_distance = 15.0f;
  const float orbit_max_distance = 100.0f;
  if (beginning()) {
    float hit_distance = intersectPlane(
      target_camera.transform().translation,
      as::mat3_basis_z(target_camera.transform().rotation),
      as::vec4(as::vec3::axis_y()));

    if (hit_distance >= 0.0f) {
      const float dist = std::min(hit_distance, orbit_max_distance);
      next_camera.focal_dist = -dist;
      next_camera.look_at =
        target_camera.transform().translation
        + as::mat3_basis_z(target_camera.transform().rotation) * dist;
    } else {
      next_camera.focal_dist = -default_orbit_distance;
      next_camera.look_at =
        target_camera.transform().translation
        + as::mat3_basis_z(target_camera.transform().rotation)
            * default_orbit_distance;
    }
  }

  if (active()) {
    // todo: need to return nested cameras to idle state when ending
    next_camera = orbit_cameras_.stepCamera(next_camera, delta_time);
  }

  if (ending()) {
    // bit of a hack... fix me
    orbit_cameras_.last_mouse_position_ = {};
    orbit_cameras_.current_mouse_position_ = {};
    orbit_cameras_.reset();

    next_camera.look_at = next_camera.transform().translation;
    next_camera.focal_dist = 0.0f;
  }

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
