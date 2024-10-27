#include "transforms-scene.h"

#include "1d-nonlinear-transformations.h"
#include "debug.h"
#include "noise.h"
#include "smooth-line.h"

#include <SDL.h>
#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <bx/timer.h>
#include <easy_iterator.h>
#include <imgui.h>
#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-line.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>
#include <thh-bgfx-debug/debug-sphere.hpp>

static float intersectPlane(
  const as::vec3& origin, const as::vec3& direction, const as::vec4& plane) {
  return -(as::vec_dot(origin, as::vec3_from_vec4(plane)) + plane.w)
       / as::vec_dot(direction, as::vec3_from_vec4(plane));
}

void transforms_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height) {
  screen_dimension = as::vec2i(width, height);

  main_view_ = main_view;
  ortho_view_ = ortho_view;

  // initial camera position and orientation
  camera.pivot = as::vec3(24.3f, 1.74f, -33.0f);
  target_camera = camera;

  const float fov = as::radians(60.0f);
  perspective_projection = as::perspective_direct3d_lh(
    fov, float(width) / float(height), 0.01f, 1000.0f);

  p0_index = curve_handles.addHandle(as::vec3(2.0f, -8.0f, 0.0f));
  p1_index = curve_handles.addHandle(as::vec3(18.0f, -8.0f, 0.0f));
  c0_index = curve_handles.addHandle(as::vec3(5.2f, -4.0f, 0.0f));
  c1_index = curve_handles.addHandle(as::vec3(8.4f, -4.0f, 0.0f));
  c2_index = curve_handles.addHandle(as::vec3(11.6f, -4.0f, 0.0f));
  c3_index = curve_handles.addHandle(as::vec3(14.8f, -4.0f, 0.0f));

  smooth_line_begin_index = curve_handles.addHandle(as::vec3::axis_x(22.0f));
  smooth_line_end_index = curve_handles.addHandle(as::vec3(25.0f, 4.0f, 0.0f));

  auto pivotFn = [this] { return pivot_translation_; };
  first_person_focus_camera.pivotFn_ = pivotFn;
  pivot_focus_camera.pivotFn_ = pivotFn;

  auto constrain_pitch = [this]() { return !tracking_; };
  first_person_rotate_camera.constrain_pitch_ = constrain_pitch;
  pivot_rotate_camera.constrain_pitch_ = constrain_pitch;

  asci::Cameras& cameras = camera_system.cameras_;
  cameras.addCamera(&first_person_rotate_camera);
  cameras.addCamera(&first_person_scroll_camera);
  cameras.addCamera(&first_person_pan_camera);
  cameras.addCamera(&first_person_translate_camera);
  cameras.addCamera(&first_person_focus_camera);
  cameras.addCamera(&pivot_camera);
  pivot_camera.pivotFn_ = pivotFn;
  pivot_camera.pivot_cameras_.addCamera(&pivot_dolly_scroll_camera);
  pivot_camera.pivot_cameras_.addCamera(&pivot_dolly_motion_camera);
  pivot_camera.pivot_cameras_.addCamera(&pivot_rotate_camera);
  pivot_camera.pivot_cameras_.addCamera(&pivot_translate_camera);
  pivot_camera.pivot_cameras_.addCamera(&pivot_pan_camera);
  pivot_camera.pivot_cameras_.addCamera(&pivot_focus_camera);
}

void transforms_scene_t::input(const SDL_Event& current_event) {
  camera_system.handleEvents(asci_sdl::sdlToInput(&current_event));

  if (current_event.type == SDL_MOUSEBUTTONDOWN) {
    if (hit_distance >= 0.0f) {
      const auto hit = ray_origin + ray_direction * hit_distance;
      curve_handles.tryBeginDrag(hit);
    }
  }
  if (current_event.type == SDL_MOUSEBUTTONUP) {
    curve_handles.clearDrag();
  }
  if (current_event.type == SDL_KEYDOWN) {
    const auto* keyboard_event = (SDL_KeyboardEvent*)&current_event;
    const auto key = keyboard_event->keysym.scancode;
    if (key == SDL_SCANCODE_R) {
      camera_transform_end = as::rigid_from_affine(camera.transform());
    }
    if (key == SDL_SCANCODE_P) {
      camera_animation_t = 0.0f;
      camera_mode = CameraMode::Animation;
      camera_transform_start = as::rigid_from_affine(camera.transform());
    }
    if (key == SDL_SCANCODE_C) {
      tracking_ = !tracking_;
      if (tracking_) {
        on_camera_transform_changed(next_stored_camera_transform_, false);
      } else {
        target_roll_ = 0.0f;
      }
    }
  }
}

void transforms_scene_t::update(
  debug_draw_t& debug_draw, const float delta_time) {
  int global_x;
  int global_y;
  SDL_GetGlobalMouseState(&global_x, &global_y);

  SDL_DisplayMode display_mode;
  SDL_GetDesktopDisplayMode(0, &display_mode);

  if (global_x >= display_mode.w - 1) {
    SDL_WarpMouseGlobal(1, global_y);
  }

  if (global_y >= display_mode.h - 1) {
    SDL_WarpMouseGlobal(global_y, 1);
  }

  if (global_x == 0) {
    SDL_WarpMouseGlobal(display_mode.w - 2, global_y);
  }

  if (global_y == 0) {
    SDL_WarpMouseGlobal(global_x, display_mode.h - 2);
  }

  int x;
  int y;
  SDL_GetMouseState(&x, &y);
  const auto world_position = as::screen_to_world(
    as::vec2i(x, y), perspective_projection, camera.view(), screen_dimension,
    as::vec2(0.0f, 1.0f));
  ray_origin = camera.transform().translation;
  ray_direction = as::vec_normalize(world_position - ray_origin);
  hit_distance =
    intersectPlane(ray_origin, ray_direction, as::vec4(as::vec3::axis_z()));

  if (curve_handles.dragging() && hit_distance > 0.0f) {
    const auto next_hit = ray_origin + ray_direction * hit_distance;
    curve_handles.updateDrag(next_hit);
  }

  static bool smoothing = true;

  ImGui::Begin("Camera");
  ImGui::PushItemWidth(70);
  ImGui::InputFloat(
    "Free Look Rotate Speed", &first_person_rotate_camera.props_.rotate_speed_);
  ImGui::InputFloat(
    "Free Look Pan Speed", &first_person_pan_camera.props_.pan_speed_);
  ImGui::InputFloat(
    "Translate Speed", &first_person_translate_camera.props_.translate_speed_);
  ImGui::Checkbox("Smoothing", &smoothing);
  ImGui::InputFloat("Look Smoothness", &smooth_props.look_smoothness_);
  ImGui::InputFloat("Move Smoothness", &smooth_props.move_smoothness_);
  ImGui::InputFloat(
    "Boost Multiplier",
    &first_person_translate_camera.props_.boost_multiplier_);
  ImGui::InputFloat("Pivot Speed", &pivot_rotate_camera.props_.rotate_speed_);
  ImGui::InputFloat(
    "Dolly Mouse Speed", &pivot_dolly_motion_camera.props_.dolly_speed_);
  ImGui::PopItemWidth();
  ImGui::Checkbox(
    "Pan Invert X", &first_person_pan_camera.props_.pan_invert_x_);
  ImGui::Checkbox(
    "Pan Invert Y", &first_person_pan_camera.props_.pan_invert_y_);
  ImGui::LabelText("Target Pitch", "%f", as::degrees(target_camera.pitch));
  ImGui::LabelText("Current Pitch", "%f", as::degrees(camera.pitch));
  ImGui::LabelText("Target Yaw", "%f", as::degrees(target_camera.yaw));
  ImGui::LabelText("Current Yaw", "%f", as::degrees(camera.yaw));
  ImGui::LabelText("Target Roll", "%f", as::degrees(target_camera.roll));
  ImGui::LabelText("Current Roll", "%f", as::degrees(camera.roll));
  ImGui::End();

  ImGui::Begin("Transforms");
  float translation_imgui[3];
  as::vec_to_arr(pivot_translation_, translation_imgui);
  ImGui::SliderFloat3("Translation", translation_imgui, -50.0f, 50.0f);
  float rotation_imgui[] = {0.0f, 0.0f, 0.0f};
  auto pivot_rotation_deg_before = as::vec_degrees(pivot_rotation_);
  as::vec_to_arr(pivot_rotation_deg_before, rotation_imgui);
  ImGui::SliderFloat3("Rotation", rotation_imgui, -360.0f, 360.0f);
  ImGui::End();

  auto pivot_translation_before = pivot_translation_;
  auto pivot_rotation_before = pivot_rotation_;

  pivot_translation_ = as::vec3_from_arr(translation_imgui);
  auto pivot_rotation_deg_after = as::vec3_from_arr(rotation_imgui);
  pivot_rotation_ = as::vec_radians(pivot_rotation_deg_after);

  if (
    (!as::vec_near(pivot_translation_before, pivot_translation_)
     || !as::vec_near(pivot_rotation_deg_before, pivot_rotation_deg_after))
    && tracking_) {
    on_camera_transform_changed(
      as::affine_from_mat3_vec3(
        as::mat3_rotation_zxy(
          pivot_rotation_.x, pivot_rotation_.y, pivot_rotation_.z),
        pivot_translation_),
      false);
  }

  as::mat4 camera_view = as::mat4::identity();
  if (camera_mode == CameraMode::Control) {
    auto target_combined_transform_before = as::affine_mul(
      as::affine_from_mat3(as::mat3_rotation_z(target_roll_)),
      target_camera.transform());

    target_camera = camera_system.stepCamera(target_camera, delta_time);

    if (tracking_) {
      auto target_combined_transform = as::affine_mul(
        as::affine_from_mat3(as::mat3_rotation_z(target_roll_)),
        target_camera.transform());

      if (
        target_combined_transform_before.rotation
          != target_combined_transform.rotation
        || target_combined_transform_before.translation
             != target_combined_transform.translation) {
        on_camera_transform_changed(target_combined_transform, true);
      }
    }

    if (smoothing) {
      camera =
        asci::smoothCamera(camera, target_camera, smooth_props, delta_time);
    } else {
      camera = target_camera;
    }

    const as::real look_rate = exp2(smooth_props.look_smoothness_);
    const as::real look_t = exp2(-look_rate * delta_time);
    roll_ = as::mix(target_roll_, roll_, look_t);

    auto combined_transform = as::affine_mul(
      as::affine_from_mat3(as::mat3_rotation_z(roll_)), camera.transform());

    camera_view = as::mat4_from_affine(as::affine_inverse(combined_transform));
  } else if (camera_mode == CameraMode::Animation) {
    auto camera_transform_current = as::rigid(
      as::quat_slerp(
        camera_transform_start.rotation, camera_transform_end.rotation,
        as::smoother_step(camera_animation_t)),
      as::vec_mix(
        camera_transform_start.translation, camera_transform_end.translation,
        as::smoother_step(camera_animation_t)));

    camera_view =
      as::mat4_from_rigid(as::rigid_inverse(camera_transform_current));

    const auto angles =
      asci::eulerAngles(as::mat3_from_quat(camera_transform_current.rotation));
    camera.pitch = angles.x;
    camera.yaw = angles.y;
    camera.offset = as::vec3::zero();
    camera.pivot = camera_transform_current.translation;
    target_camera = camera;

    if (camera_animation_t >= 1.0f) {
      camera_mode = CameraMode::Control;
    }

    camera_animation_t =
      as::clamp(camera_animation_t + (delta_time / 1.0f), 0.0f, 1.0f);
  }

  float view[16];
  as::mat_to_arr(camera_view, view);

  float proj_p[16];
  as::mat_to_arr(perspective_projection, proj_p);
  bgfx::setViewTransform(main_view_, view, proj_p);

  const float smooth_stop_start_mix = as::mix(
    nlt::smoothStart2(debug.smooth_stop_start_mix_t),
    nlt::smoothStop2(debug.smooth_stop_start_mix_t),
    debug.smooth_stop_start_mix_t);
  const float smoother_stop_start_mix = as::mix(
    nlt::smoothStart3(debug.smooth_stop_start_mix_t),
    nlt::smoothStop3(debug.smooth_stop_start_mix_t),
    as::smooth_step(debug.smooth_stop_start_mix_t));
  const float bezier3_1d =
    nlt::bezier3(
      as::vec3::zero(), as::vec3::axis_x(1.0f), as::vec3::axis_x(0.0f),
      as::vec3::axis_x(1.0f), debug.smooth_stop_start_mix_t)
      .x;
  const float bezier5_1d =
    nlt::bezier5(
      as::vec3::zero(), as::vec3::axis_x(1.0f), as::vec3::axis_x(0.0f),
      as::vec3::axis_x(0.0f), as::vec3::axis_x(1.0f), as::vec3::axis_x(1.0f),
      debug.smooth_stop_start_mix_t)
      .x;

  ImGui::Begin("Curves");
  ImGui::Checkbox("Linear", &debug.linear);
  ImGui::Checkbox("Smooth Step", &debug.smooth_step);
  ImGui::Checkbox("Smoother Step", &debug.smoother_step);
  ImGui::Checkbox("Smooth Stop Start Mix 2", &debug.smooth_stop_start_mix2);
  ImGui::Checkbox("Smooth Stop Start Mix 3", &debug.smooth_stop_start_mix3);
  ImGui::SliderFloat("t", &debug.smooth_stop_start_mix_t, 0.0f, 1.0f);
  ImGui::Text("Smooth Step: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", as::smooth_step(debug.smooth_stop_start_mix_t));
  ImGui::Text("Smooth Stop Start Mix 2: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", smooth_stop_start_mix);
  ImGui::Text("Bezier3 1d: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", bezier3_1d);
  ImGui::Text("Smoother Step: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", as::smoother_step(debug.smooth_stop_start_mix_t));
  ImGui::Text("Smooth Stop Start Mix 3: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", smoother_stop_start_mix);
  ImGui::Text("Bezier5 1d: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", bezier5_1d);
  ImGui::Checkbox("Smooth Start 2", &debug.smooth_start2);
  ImGui::Checkbox("Smooth Start 3", &debug.smooth_start3);
  ImGui::Checkbox("Smooth Start 4", &debug.smooth_start4);
  ImGui::Checkbox("Smooth Start 5", &debug.smooth_start5);
  ImGui::Checkbox("Smooth Stop 2", &debug.smooth_stop2);
  ImGui::Checkbox("Smooth Stop 3", &debug.smooth_stop3);
  ImGui::Checkbox("Smooth Stop 4", &debug.smooth_stop4);
  ImGui::Checkbox("Smooth Stop 5", &debug.smooth_stop5);
  ImGui::Checkbox("Arch 2", &debug.arch2);
  ImGui::Checkbox("Smooth Start Arch 3", &debug.smooth_start_arch3);
  ImGui::Checkbox("Smooth Stop Arch 3", &debug.smooth_stop_arch3);
  ImGui::Checkbox("Smooth Step Arch 4", &debug.smooth_step_arch4);
  ImGui::Checkbox("Normalized Bezier 2", &debug.normalized_bezier2);
  ImGui::Checkbox("Bezier Smooth Step", &debug.bezier_smooth_step);
  ImGui::Checkbox("Normalized Bezier 3", &debug.normalized_bezier3);
  ImGui::Checkbox("Normalized Bezier 4", &debug.normalized_bezier4);
  ImGui::Checkbox("Normalized Bezier 5", &debug.normalized_bezier5);
  ImGui::SliderFloat("b", &debug.normalized_bezier_b, 0.0f, 1.0f);
  ImGui::SliderFloat("c", &debug.normalized_bezier_c, 0.0f, 1.0f);
  ImGui::SliderFloat("d", &debug.normalized_bezier_d, 0.0f, 1.0f);
  ImGui::SliderFloat("e", &debug.normalized_bezier_e, 0.0f, 1.0f);

  // draw alignment transform
  drawTransform(
    *debug_draw.debug_lines, as::affine_from_rigid(camera_transform_end));

  drawTransform(*debug_draw.debug_lines, as::affine_from_vec3(camera.pivot));
  drawTransform(*debug_draw.debug_lines, next_stored_camera_transform_);

  drawGrid(*debug_draw.debug_lines, camera.pivot);

  const auto p0 = curve_handles.getHandle(p0_index);
  const auto p1 = curve_handles.getHandle(p1_index);
  const auto c0 = curve_handles.getHandle(c0_index);
  const auto c1 = curve_handles.getHandle(c1_index);
  const auto c2 = curve_handles.getHandle(c2_index);
  const auto c3 = curve_handles.getHandle(c3_index);

  std::vector<std::vector<as::vec3>> points = {
    {},
    {p0, c0, c0, p1},
    {p0, c0, c0, c1, c1, p1},
    {p0, c0, c0, c1, c1, c2, c2, p1},
    {p0, c0, c0, c1, c1, c2, c2, c3, c3, p1}};

  static int order = 4;
  static const char* orders[] = {"First", "Second", "Third", "Fourth", "Fifth"};

  ImGui::Combo("Curve Order", &order, orders, std::size(orders));

  // control lines
  for (as::index i = 0; i < points[order].size(); i += 2) {
    debug_draw.debug_lines->addLine(
      points[order][i], points[order][i + 1], 0xffaaaaaa);
  }

  // control handles
  for (as::index index = 0; index < order + 2; ++index) {
    const auto translation = as::mat4_from_mat3_vec3(
      as::mat3::identity(), curve_handles.getHandle(index));
    const auto scale =
      as::mat4_from_mat3(as::mat3_scale(dbg::CurveHandles::HandleRadius));

    debug_draw.debug_circles->addWireCircle(
      as::mat_mul(scale, translation), 0xff000000);
  }

  const auto line_granularity = 50;
  const auto line_length = 20.0f;
  for (auto i = 0; i < line_granularity; ++i) {
    float begin = i / float(line_granularity);
    float end = float(i + 1) / float(line_granularity);

    float x_begin = begin * line_length;
    float x_end = end * line_length;

    const auto sample_curve =
      [line_length, begin, end, &debug_draw, x_begin,
       x_end](auto fn, const uint32_t col = 0xff000000) {
        debug_draw.debug_lines->addLine(
          as::vec3(x_begin, as::mix(0.0f, line_length, fn(begin)), 0.0f),
          as::vec3(x_end, as::mix(0.0f, line_length, fn(end)), 0.0f), col);
      };

    if (order == 0) {
      debug_draw.debug_lines->addLine(
        nlt::bezier1(p0, p1, begin), nlt::bezier1(p0, p1, end), 0xff000000);
    }

    if (order == 1) {
      debug_draw.debug_lines->addLine(
        nlt::bezier2(p0, p1, c0, begin), nlt::bezier2(p0, p1, c0, end),
        0xff000000);
    }

    if (order == 2) {
      debug_draw.debug_lines->addLine(
        nlt::bezier3(p0, p1, c0, c1, begin), nlt::bezier3(p0, p1, c0, c1, end),
        0xff000000);
    }

    if (order == 3) {
      debug_draw.debug_lines->addLine(
        nlt::bezier4(p0, p1, c0, c1, c2, begin),
        nlt::bezier4(p0, p1, c0, c1, c2, end), 0xff000000);
    }

    if (order == 4) {
      debug_draw.debug_lines->addLine(
        nlt::bezier5(p0, p1, c0, c1, c2, c3, begin),
        nlt::bezier5(p0, p1, c0, c1, c2, c3, end), 0xff000000);
    }

    if (debug.linear) {
      sample_curve([](const float a) { return a; });
    }

    if (debug.arch2) {
      sample_curve(nlt::arch2);
    }

    if (debug.smooth_start_arch3) {
      sample_curve(nlt::smoothStartArch3);
    }

    if (debug.smooth_stop_arch3) {
      sample_curve(nlt::smoothStopArch3);
    }

    if (debug.smooth_step_arch4) {
      sample_curve(nlt::smoothStepArch4);
    }

    if (debug.smooth_step) {
      sample_curve(as::smooth_step<float>);
    }

    if (debug.smoother_step) {
      sample_curve(as::smoother_step<float>);
    }

    if (debug.smooth_stop_start_mix2) {
      sample_curve(nlt::smoothStepMixed);
      sample_curve(
        [this](const float sample) {
          return nlt::smoothStepMixer(sample, debug.smooth_stop_start_mix_t);
        },
        0xff00ff00);
    }

    if (debug.smooth_stop_start_mix3) {
      sample_curve(nlt::smootherStepMixed);
      sample_curve(
        [this](const float sample) {
          return nlt::smootherStepMixer(sample, debug.smooth_stop_start_mix_t);
        },
        0xff00ff00);
    }

    if (debug.smooth_start2) {
      sample_curve(nlt::smoothStart2);
    }

    if (debug.smooth_start3) {
      sample_curve(nlt::smoothStart3);
    }

    if (debug.smooth_start4) {
      sample_curve(nlt::smoothStart4);
    }

    if (debug.smooth_start5) {
      sample_curve(nlt::smoothStart5);
    }

    if (debug.smooth_stop2) {
      sample_curve(nlt::smoothStop2);
    }

    if (debug.smooth_stop3) {
      sample_curve(nlt::smoothStop3);
    }

    if (debug.smooth_stop4) {
      sample_curve(nlt::smoothStop4);
    }

    if (debug.smooth_stop5) {
      sample_curve(nlt::smoothStop5);
    }

    if (debug.bezier_smooth_step) {
      sample_curve(nlt::bezierSmoothStep);
    }

    if (debug.normalized_bezier2) {
      sample_curve(
        [this](const float sample) {
          return nlt::normalizedBezier2(debug.normalized_bezier_b, sample);
        },
        0xff0000ff);
    }

    if (debug.normalized_bezier3) {
      sample_curve(
        [this](const float sample) {
          return nlt::normalizedBezier3(
            debug.normalized_bezier_b, debug.normalized_bezier_c, sample);
        },
        0xff0000ff);
    }

    if (debug.normalized_bezier4) {
      sample_curve(
        [this](const float sample) {
          return nlt::normalizedBezier4(
            debug.normalized_bezier_b, debug.normalized_bezier_c,
            debug.normalized_bezier_d, sample);
        },
        0xff0000ff);
    }

    if (debug.normalized_bezier5) {
      sample_curve(
        [this](const float sample) {
          return nlt::normalizedBezier5(
            debug.normalized_bezier_b, debug.normalized_bezier_c,
            debug.normalized_bezier_d, debug.normalized_bezier_e, sample);
        },
        0xff0000ff);
    }
  }

  // animation begin
  static float direction = 1.0f;
  static float time = 2.0f;
  static float t = 0.0f;
  t += delta_time / time * direction;
  if (t >= 1.0f || t <= 0.0f) {
    t = as::clamp(t, 0.0f, 1.0f);
    direction *= -1.0f;
  }

  const auto start = as::vec3(2.0f, -1.5f, 0.0f);
  const auto end = as::vec3(18.0f, -1.5f, 0.0f);

  debug_draw.debug_lines->addLine(start, end, 0xff000000);

  // draw smooth line handles
  for (auto index : {smooth_line_begin_index, smooth_line_end_index}) {
    const auto translation = as::mat4_from_mat3_vec3(
      as::mat3::identity(), curve_handles.getHandle(index));
    const auto scale =
      as::mat4_from_mat3(as::mat3_scale(dbg::CurveHandles::HandleRadius));

    debug_draw.debug_circles->addWireCircle(
      as::mat_mul(scale, translation), 0xff000000);
  }

  // draw smooth line
  const auto smooth_line_begin =
    curve_handles.getHandle(smooth_line_begin_index);
  const auto smooth_line_end = curve_handles.getHandle(smooth_line_end_index);

  auto smooth_line = dbg::SmoothLine(debug_draw.debug_lines);
  smooth_line.draw(smooth_line_begin, smooth_line_end);

  static float (*interpolations[])(float) = {
    [](float t) { return t; }, as::smooth_step,   as::smoother_step,
    nlt::smoothStart2,         nlt::smoothStart3, nlt::smoothStart4,
    nlt::smoothStart5,         nlt::smoothStop2,  nlt::smoothStop3,
    nlt::smoothStop4,          nlt::smoothStop5};

  static int item = 0;
  static const char* types[] = {
    "Linear",         "Smooth Step",    "Smoother Step",  "Smooth Start 2",
    "Smooth Start 3", "Smooth Start 4", "Smooth Start 5", "Smooth Stop 2",
    "Smooth Stop 3",  "Smooth Stop 4",  "Smooth Stop 5",
  };

  ImGui::Combo("Interpolation", &item, types, std::size(types));
  ImGui::End();

  const auto horizontal_position =
    as::vec_mix(start, end, interpolations[item](t));

  const auto translation =
    as::mat4_from_mat3_vec3(as::mat3::identity(), horizontal_position);
  const auto scale =
    as::mat4_from_mat3(as::mat3_scale(dbg::CurveHandles::HandleRadius));

  debug_draw.debug_circles->addWireCircle(
    as::mat_mul(scale, translation), 0xff000000);

  const auto curve_position = [p0, p1, c0, c1, c2, c3] {
    if (order == 0) {
      return nlt::bezier1(p0, p1, t);
    }
    if (order == 1) {
      return nlt::bezier2(p0, p1, c0, t);
    }
    if (order == 2) {
      return nlt::bezier3(p0, p1, c0, c1, t);
    }
    if (order == 3) {
      return nlt::bezier4(p0, p1, c0, c1, c2, t);
    }
    if (order == 4) {
      return nlt::bezier5(p0, p1, c0, c1, c2, c3, t);
    }
    return as::vec3::zero();
  }();

  debug_draw.debug_circles->addWireCircle(
    as::mat_mul(
      as::mat4_from_mat3(as::mat3_scale(dbg::CurveHandles::HandleRadius)),
      as::mat4_from_mat3_vec3(as::mat3::identity(), curve_position)),
    0xff000000);
  // animation end

  // scale point mini-scene
  [&debug_draw]() {
    /// https://blog.demofox.org/2024/10/15/scaling-points-in-a-specific-direction/
    ImGui::Begin("Scaling");
    static float direction[3] = {3.0f, 1.0f, 0.0f};
    static float scale_factor = 4.0f;
    ImGui::SliderFloat3("Direction", direction, -1.0f, 1.0f);
    ImGui::SliderFloat("Scale", &scale_factor, 1.0f, 10.0f);
    ImGui::End();

    // scene offset
    const as::mat4 scale_scene_transform =
      as::mat4_from_vec3(as::vec3(-22.0f, 5.0f, 0.0f));

    float rot = 0.0f;
    const int32_t circle_count = 100;
    const float circle_size = 0.2f;
    const float increment = as::k_tau / static_cast<float>(circle_count - 1);
    for (size_t i = 1; i < circle_count; ++i) {
      // define circle
      const auto local_position =
        as::vec3(std::cos(rot), std::sin(rot), 0.0f) * 2.0f;
      const auto world_position =
        scale_scene_transform * as::vec4(local_position, 1.0f);

      // show original circle
      debug_draw.debug_circles->addSolidCircle(
        as::mat4_from_mat3_vec3(
          as::mat3_scale(circle_size), as::vec3_from_vec4(world_position)),
        0xffff0000);

      // core scaling operation
      const as::mat3 rotation = as::mat_transpose(
        as::orthonormal_basis(as::vec_normalize(as::vec3_from_ptr(direction))));
      const as::mat3 inverse_rotation = as::mat_transpose(rotation);
      const as::mat3 scale = as::mat3_scale(scale_factor, 1.0f, 1.0f);
      const as::mat3 transform = inverse_rotation * scale * rotation;
      const as::vec4 world_scale_position =
        scale_scene_transform
        * as::vec4_translation(transform * local_position);

      // show scaled circle
      debug_draw.debug_circles->addSolidCircle(
        as::mat4_from_mat3_vec3(
          as::mat3_scale(circle_size),
          as::vec3_from_vec4(world_scale_position) + as::vec3::axis_z(0.05f)),
        0xff0000ff);

      rot += increment;
    }
  }();

  // screen space drawing
  float view_o[16];
  as::mat_to_arr(as::mat4::identity(), view_o);

  const as::mat4 orthographic_projection = as::ortho_direct3d_lh(
    0.0f, screen_dimension.x, screen_dimension.y, 0.0f, 0.0f, 1.0f);

  float proj_o[16];
  as::mat_to_arr(orthographic_projection, proj_o);
  bgfx::setViewTransform(ortho_view_, view_o, proj_o);

  const auto start_screen = as::world_to_screen(
    start, perspective_projection, camera.view(), screen_dimension);
  const auto end_screen = as::world_to_screen(
    end, perspective_projection, camera.view(), screen_dimension);

  debug_draw.debug_lines_screen->addLine(
    as::vec3(start_screen.x, start_screen.y - 10.0f, 0.0f),
    as::vec3(start_screen.x, start_screen.y + 10.0f, 0.0f), 0xff000000);
  debug_draw.debug_lines_screen->addLine(
    as::vec3(end_screen.x, end_screen.y - 10.0f, 0.0f),
    as::vec3(end_screen.x, end_screen.y + 10.0f, 0.0f), 0xff000000);

  ImGui::Begin("Noise");
  static float noise1_freq = 0.5f;
  ImGui::SliderFloat("Noise 1 Freq", &noise1_freq, 0.0f, 5.0f);
  static float noise1_amp = 1.25f;
  ImGui::SliderFloat("Noise 1 Amp", &noise1_amp, 0.0f, 5.0f);
  static int noise1_offset = 0;
  ImGui::SliderInt("Noise 1 Offset", &noise1_offset, 0, 100);
  static float noise2_freq = 2.0f;
  ImGui::SliderFloat("Noise 2 Freq", &noise2_freq, 0.0f, 5.0f);
  static float noise2_amp = 0.5f;
  ImGui::SliderFloat("Noise 2 Amp", &noise2_amp, 0.0f, 5.0f);
  static int noise2_offset = 1;
  ImGui::SliderInt("Noise 2 Offset", &noise2_offset, 0, 100);
  static float noise3_freq = 4.0f;
  ImGui::SliderFloat("Noise 3 Freq", &noise3_freq, 0.0f, 5.0f);
  static float noise3_amp = 0.2f;
  ImGui::SliderFloat("Noise 3 Amp", &noise3_amp, 0.0f, 5.0f);
  static int noise3_offset = 2;
  ImGui::SliderInt("Noise 3 Offset", &noise3_offset, 0, 100);
  static float noise2d_freq = 1.0f;
  ImGui::SliderFloat("Noise 2d Freq", &noise2d_freq, 0.0f, 10.0f);
  static float noise2d_amp = 1.0f;
  ImGui::SliderFloat("Noise 2d Amp", &noise2d_amp, 0.0f, 10.0f);
  static int noise2d_offset = 0;
  ImGui::SliderInt("Noise 2d Offset", &noise2d_offset, 0, 100);
  static float noise2d_position[] = {0.0f, 0.0f};
  ImGui::SliderFloat2("Noise 2d Position", noise2d_position, -10.0f, 10.0f);
  static bool draw_gradients = false;
  ImGui::Checkbox("Draw Gradients", &draw_gradients);
  ImGui::End();

  // draw random noise
  for (int64_t i = 0; i < 160; ++i) {
    const float offset = (i * 0.1f) + 2.0f;
    debug_draw.debug_lines->addLine(
      as::vec3(offset, -10.0f, 0.0f),
      as::vec3(offset, -10.0f + ns::noise1dZeroToOne(i), 0.0f), 0xff000000);
    debug_draw.debug_lines->addLine(
      as::vec3(offset, -12.0f, 0.0f),
      as::vec3(offset, -12.0f + ns::noise1dMinusOneToOne(i), 0.0f), 0xff000000);
  }

  // draw perlin noise
  for (int64_t i = 0; i < 320; ++i) {
    const float offset = (i * 0.05f) + 2.0f;
    const float next_offset = ((float(i + 1)) * 0.05f) + 2.0f;
    debug_draw.debug_lines->addLine(
      as::vec3(
        offset,
        -14.0f
          + ns::perlinNoise1d(offset * noise1_freq, noise1_offset) * noise1_amp
          + ns::perlinNoise1d(offset * noise2_freq, noise2_offset) * noise2_amp
          + ns::perlinNoise1d(offset * noise3_freq, noise3_offset) * noise3_amp,
        0.0f),
      as::vec3(
        next_offset,
        -14.0f
          + ns::perlinNoise1d(next_offset * noise1_freq, noise1_offset)
              * noise1_amp
          + ns::perlinNoise1d(next_offset * noise2_freq, noise2_offset)
              * noise2_amp
          + ns::perlinNoise1d(next_offset * noise3_freq, noise3_offset)
              * noise3_amp,
        0.0f),
      0xff000000);
  }

  const auto noise_position = as::vec2_from_arr(noise2d_position);
  const auto starting_offset = as::vec3::axis_x(-12.0f);
  for (size_t r = 0; r < 100; ++r) {
    for (size_t c = 0; c < 100; ++c) {
      const as::vec2 p = as::vec2(float(c) * 0.1f, float(r) * 0.1f);
      const float grey = as::clamp(
        ns::perlinNoise2d((p + noise_position) * noise2d_freq, noise2d_offset)
            * noise2d_amp
          + 0.5f,
        0.0f, 1.0f);

      if (draw_gradients && c % 10 == 0 && r % 10 == 0) {
        const as::vec2 p0 = as::vec_floor(p);
        const as::vec2 g0 = ns::gradient(ns::angle(p0, noise2d_offset));
        debug_draw.debug_lines->addLine(
          starting_offset + as::vec3(p0) / noise2d_freq,
          starting_offset + (as::vec3(p0) + as::vec3(g0)) / noise2d_freq,
          0xff000000);
      }

      const auto translation =
        as::mat4_from_vec3(as::vec3(p) + starting_offset);
      const auto scale = as::mat4_from_mat3(as::mat3_scale(0.1f));

      debug_draw.debug_quads->addQuad(
        as::mat_mul(scale, translation),
        dbg::encodeColorAbgr(dbg::NormalizedRgba(grey, 1.0f)));
    }
  }

  const as::rigid rigid_transformation(
    as::quat_rotation_zxy(as::vec_radians(as::vec_from_arr(rotation_imgui))),
    as::vec3_from_arr(translation_imgui));
  const as::vec3 next_position_rigid =
    as::rigid_transform_pos(rigid_transformation, as::vec3::axis_z(0.5f));

  const as::affine affine_transformation(
    as::mat3_rotation_zxy(as::vec_radians(as::vec_from_arr(rotation_imgui))),
    as::vec3_from_arr(translation_imgui));
  const as::vec3 next_position_affine =
    as::affine_transform_pos(affine_transformation, as::vec3::axis_z(0.5f));

  next_stored_camera_transform_ = affine_transformation;

  const as::rigid next_rigid =
    as::rigid_mul(as::rigid(as::quat::identity()), rigid_transformation);
  const as::affine next_affine =
    as::affine_mul(as::affine(as::mat3::identity()), affine_transformation);

  as::rigid another_rigid =
    as::rigid_mul(as::rigid(as::vec3::axis_z(-0.5f)), next_rigid);
  as::affine another_affine =
    as::affine_mul(as::affine(as::vec3::axis_z(-0.5f)), next_affine);

  debug_draw.debug_cubes->addCube(as::mat4_from_rigid(next_rigid), 0xffffffff);
  debug_draw.debug_cubes->addCube(
    as::mat4_from_affine(next_affine), 0xff000000);

  debug_draw.debug_spheres->addSphere(
    as::mat4_from_vec3(next_position_rigid)
      * as::mat4_from_mat3(as::mat3_scale(0.2f)),
    0xffffffff);
  debug_draw.debug_spheres->addSphere(
    as::mat4_from_vec3(next_position_affine)
      * as::mat4_from_mat3(as::mat3_scale(0.2f)),
    0xff000000);

  debug_draw.debug_spheres->addSphere(
    as::mat4_from_rigid(another_rigid)
      * as::mat4_from_mat3(as::mat3_scale(0.2f)),
    0xffffffff);
  debug_draw.debug_spheres->addSphere(
    as::mat4_from_affine(another_affine)
      * as::mat4_from_mat3(as::mat3_scale(0.2f)),
    0xff000000);
}

void transforms_scene_t::on_camera_transform_changed(
  const as::affine& camera_transform, bool internal) {
  const auto angles = asci::eulerAngles(camera_transform.rotation);

  if (internal) {
    pivot_translation_ = camera_transform.translation;
    pivot_rotation_ = angles;
  } else {
    target_camera.pitch = angles.x;
    target_camera.yaw = angles.y;
    target_camera.offset = as::vec3::zero();
    target_camera.pivot = camera_transform.translation;
    target_roll_ = angles.z;
  }
}
