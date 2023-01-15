#pragma once

#include "curve-handles.h"
#include "fps.h"
#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>
#include <as/as-math-ops.hpp>
#include <bgfx/bgfx.h>
#include <thh-bgfx-debug/debug-shader.hpp>

union SDL_Event;

struct debug_settings_t
{
  bool linear = true;
  bool smooth_step = true;
  bool smoother_step = true;
  bool smooth_stop_start_mix2 = true;
  bool smooth_stop_start_mix3 = true;
  bool smooth_start2 = true;
  bool smooth_start3 = true;
  bool smooth_start4 = true;
  bool smooth_start5 = true;
  bool smooth_stop2 = true;
  bool smooth_stop3 = true;
  bool smooth_stop4 = true;
  bool smooth_stop5 = true;
  bool arch2 = true;
  bool smooth_start_arch3 = true;
  bool smooth_stop_arch3 = true;
  bool smooth_step_arch4 = true;
  bool bezier_smooth_step = true;
  bool normalized_bezier2 = true;
  bool normalized_bezier3 = true;
  bool normalized_bezier4 = true;
  bool normalized_bezier5 = true;
  float smooth_stop_start_mix_t = 0.0f;
  float normalized_bezier_b = 0.0f;
  float normalized_bezier_c = 0.0f;
  float normalized_bezier_d = 0.0f;
  float normalized_bezier_e = 0.0f;
};

enum class CameraMode
{
  Control,
  Animation
};

struct transforms_scene_t : public scene_t
{
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override {}

  void on_camera_transform_changed(
    const as::affine& camera_transform, bool internal);

  as::vec2i screen_dimension{};
  bgfx::ViewId main_view_;
  bgfx::ViewId ortho_view_;

  as::mat4 perspective_projection;

  debug_settings_t debug;

  dbg::CurveHandles curve_handles;
  as::index p0_index;
  as::index p1_index;
  as::index c0_index;
  as::index c1_index;
  as::index c2_index;
  as::index c3_index;
  as::index smooth_line_begin_index;
  as::index smooth_line_end_index;

  asci::SmoothProps smooth_props;

  asc::Camera camera;
  asc::Camera target_camera;

  asci::RotateCameraInput first_person_rotate_camera{asci::MouseButton::Right};
  asci::PanCameraInput first_person_pan_camera{
    asci::MouseButton::Middle, asci::lookPan, asci::translatePivot};
  asci::TranslateCameraInput first_person_translate_camera{
    asci::lookTranslation, asci::translatePivot};
  asci::ScrollTranslationCameraInput first_person_scroll_camera{};
  asci::FocusCameraInput first_person_focus_camera{
    asci::KeyboardButton::F, asci::focusLook};

  asci::PivotCameraInput pivot_camera{};
  asci::PivotDollyScrollCameraInput pivot_dolly_scroll_camera;
  asci::RotateCameraInput pivot_rotate_camera{asci::MouseButton::Left};
  asci::TranslateCameraInput pivot_translate_camera{
    asci::pivotTranslation, asci::translateOffset};
  asci::PivotDollyMotionCameraInput pivot_dolly_motion_camera{
    asci::MouseButton::Right};
  asci::PanCameraInput pivot_pan_camera{
    asci::MouseButton::Middle, asci::lookPan, asci::translateOffset};
  asci::FocusCameraInput pivot_focus_camera{
    asci::KeyboardButton::F, asci::focusPivot};

  asci::Cameras cameras;
  asci::CameraSystem camera_system;

  as::rigid camera_transform_start = as::rigid(as::quat::identity());
  as::rigid camera_transform_end = as::rigid(as::quat::identity());

  float camera_animation_t = 0.0f;
  CameraMode camera_mode = CameraMode::Control;

  float hit_distance = -1.0f;
  as::vec3 ray_origin;
  as::vec3 ray_direction;

  as::vec3 pivot_translation_ = as::vec3(31.0f, 5.0f, -17.0f);
  as::vec3 pivot_rotation_ = as::vec3::zero();
  float roll_ = 0.0f;
  float target_roll_ = 0.0f;

  as::affine next_stored_camera_transform_ = as::affine::identity();
  bool tracking_ = false;
};
