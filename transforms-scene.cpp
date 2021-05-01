#include "transforms-scene.h"

#include "1d-nonlinear-transformations.h"
#include "noise.h"
#include "sdl-camera-input.h"
#include "smooth-line.h"

#include <SDL.h>
#include <as/as-view.hpp>
#include <bx/timer.h>
#include <easy_iterator.h>
#include <imgui.h>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-line.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>
#include <thh-bgfx-debug/debug-sphere.hpp>

static void drawTransform(
  dbg::DebugLines& debug_lines, const as::affine& affine,
  const float alpha = 1.0f)
{
  const uint8_t alpha_8 = static_cast<uint8_t>(255.0f * alpha);
  const uint32_t alpha_32 = alpha_8 << 24;

  const auto& translation = affine.translation;
  debug_lines.addLine(
    translation, translation + as::mat3_basis_x(affine.rotation),
    0x000000ff | alpha_32);
  debug_lines.addLine(
    translation, translation + as::mat3_basis_y(affine.rotation),
    0x0000ff00 | alpha_32);
  debug_lines.addLine(
    translation, translation + as::mat3_basis_z(affine.rotation),
    0x00ff0000 | alpha_32);
}

static float intersectPlane(
  const as::vec3& origin, const as::vec3& direction, const as::vec4& plane)
{
  return -(as::vec_dot(origin, as::vec3_from_vec4(plane)) + plane.w)
       / as::vec_dot(direction, as::vec3_from_vec4(plane));
}

// https://www.geometrictools.com/Documentation/EulerAngles.pdf
std::tuple<float, float, float> eulerAngles(const as::mat3& orientation)
{
  float x;
  float y;
  float z;

  // 2.4 Factor as RyRzRx
  if (orientation[as::mat3_rc(1, 0)] < 1.0f) {
    if (orientation[as::mat3_rc(1, 0)] > -1.0f) {
      x = std::atan2(
        -orientation[as::mat3_rc(1, 2)], orientation[as::mat3_rc(1, 1)]);
      y = std::atan2(
        -orientation[as::mat3_rc(2, 0)], orientation[as::mat3_rc(0, 0)]);
      z = std::asin(orientation[as::mat3_rc(1, 0)]);
    } else {
      x = 0.0f;
      y = -std::atan2(
        orientation[as::mat3_rc(2, 1)], orientation[as::mat3_rc(2, 2)]);
      z = -as::k_pi * 0.5f;
    }
  } else {
    x = 0.0f;
    y = std::atan2(
      orientation[as::mat3_rc(2, 1)], orientation[as::mat3_rc(2, 2)]);
    z = as::k_pi * 0.5f;
  }

  return {x, y, z};
}

void setup(transforms_scene_t& scene, uint16_t width, uint16_t height)
{
  scene.screen_dimension = as::vec2i(width, height);

  // cornflower clear color
  bgfx::setViewClear(
    scene.main_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
  bgfx::setViewRect(scene.main_view, 0, 0, width, height);
  bgfx::setViewClear(scene.ortho_view, BGFX_CLEAR_DEPTH);
  bgfx::setViewRect(scene.ortho_view, 0, 0, width, height);
  // dummy draw call to make sure main_view is cleared if no other draw calls
  // are submitted
  bgfx::touch(scene.main_view);
  bgfx::touch(scene.ortho_view);

  scene.simple_program.init(dbg::SimpleEmbeddedShaderArgs);
  scene.instance_program.init(dbg::InstanceEmbeddedShaderArgs);

  // initial camera position and orientation
  scene.camera.look_at = as::vec3(24.3f, 1.74f, -33.0f);
  scene.target_camera = scene.camera;

  const float fov = as::radians(60.0f);
  scene.perspective_projection =
    as::perspective_d3d_lh(fov, float(width) / float(height), 0.01f, 1000.0f);

  scene.p0_index = scene.curve_handles.addHandle(as::vec3(2.0f, -8.0f, 0.0f));
  scene.p1_index = scene.curve_handles.addHandle(as::vec3(18.0f, -8.0f, 0.0f));
  scene.c0_index = scene.curve_handles.addHandle(as::vec3(5.2f, -4.0f, 0.0f));
  scene.c1_index = scene.curve_handles.addHandle(as::vec3(8.4f, -4.0f, 0.0f));
  scene.c2_index = scene.curve_handles.addHandle(as::vec3(11.6f, -4.0f, 0.0f));
  scene.c3_index = scene.curve_handles.addHandle(as::vec3(14.8f, -4.0f, 0.0f));

  scene.smooth_line_begin_index =
    scene.curve_handles.addHandle(as::vec3::axis_x(22.0f));
  scene.smooth_line_end_index =
    scene.curve_handles.addHandle(as::vec3(25.0f, 4.0f, 0.0f));

  scene.prev = bx::getHPCounter();

  scene.orbit_camera.orbit_cameras_.addCamera(&scene.orbit_rotate_camera);
  scene.orbit_camera.orbit_cameras_.addCamera(&scene.orbit_translate_camera);
  scene.orbit_camera.orbit_cameras_.addCamera(&scene.orbit_dolly_wheel_camera);
  scene.orbit_camera.orbit_cameras_.addCamera(&scene.orbit_dolly_move_camera);
  scene.orbit_camera.orbit_cameras_.addCamera(&scene.orbit_pan_camera);

  scene.cameras.addCamera(&scene.first_person_rotate_camera);
  scene.cameras.addCamera(&scene.first_person_pan_camera);
  scene.cameras.addCamera(&scene.first_person_translate_camera);
  scene.cameras.addCamera(&scene.first_person_wheel_camera);
  scene.cameras.addCamera(&scene.orbit_camera);

  scene.camera_system.cameras_ = scene.cameras;
}

void input(transforms_scene_t& scene, const SDL_Event& current_event)
{
  if (current_event.type == SDL_QUIT) {
    scene.quit = true;
    return;
  }

  scene.camera_system.handleEvents(sdlToInput(&current_event));

  if (current_event.type == SDL_MOUSEBUTTONDOWN) {
    if (scene.hit_distance >= 0.0f) {
      const auto hit =
        scene.ray_origin + scene.ray_direction * scene.hit_distance;
      scene.curve_handles.tryBeginDrag(hit);
    }
  }
  if (current_event.type == SDL_MOUSEBUTTONUP) {
    scene.curve_handles.clearDrag();
  }
  if (current_event.type == SDL_KEYDOWN) {
    const auto* keyboard_event = (SDL_KeyboardEvent*)&current_event;
    const auto key = keyboard_event->keysym.scancode;
    if (key == SDL_SCANCODE_R) {
      scene.camera_transform_end =
        as::rigid_from_affine(scene.camera.transform());
    }
    if (key == SDL_SCANCODE_P) {
      scene.camera_animation_t = 0.0f;
      scene.camera_mode = CameraMode::Animation;
      scene.camera_transform_start =
        as::rigid_from_affine(scene.camera.transform());
    }
  }
}

void update(transforms_scene_t& scene)
{
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
    as::vec2i(x, y), scene.perspective_projection, scene.camera.view(),
    scene.screen_dimension);
  scene.ray_origin = scene.camera.transform().translation;
  scene.ray_direction = as::vec_normalize(world_position - scene.ray_origin);
  scene.hit_distance = intersectPlane(
    scene.ray_origin, scene.ray_direction, as::vec4(as::vec3::axis_z()));

  dbg::DebugCircles debug_circles(
    scene.main_view, scene.instance_program.handle());
  dbg::DebugSpheres debug_spheres(debug_circles);

  if (scene.curve_handles.dragging() && scene.hit_distance > 0.0f) {
    const auto next_hit =
      scene.ray_origin + scene.ray_direction * scene.hit_distance;
    scene.curve_handles.updateDrag(next_hit);
  }

  ImGui::Begin("Camera");
  ImGui::PushItemWidth(70);
  ImGui::InputFloat(
    "Free Look Rotate Speed",
    &scene.first_person_rotate_camera.props_.rotate_speed_);
  ImGui::InputFloat(
    "Free Look Pan Speed", &scene.first_person_pan_camera.props_.pan_speed_);
  ImGui::InputFloat(
    "Translate Speed",
    &scene.first_person_translate_camera.props_.translate_speed_);
  ImGui::InputFloat("Look Smoothness", &scene.smooth_props.look_smoothness_);
  ImGui::InputFloat("Move Smoothness", &scene.smooth_props.move_smoothness_);
  ImGui::InputFloat(
    "Boost Multiplier",
    &scene.first_person_translate_camera.props_.boost_multiplier_);
  ImGui::InputFloat(
    "Default Orbit Distance",
    &scene.orbit_camera.props_.default_orbit_distance_);
  ImGui::InputFloat(
    "Max Orbit Distance", &scene.orbit_camera.props_.max_orbit_distance_);
  ImGui::InputFloat(
    "Orbit Speed", &scene.orbit_rotate_camera.props_.rotate_speed_);
  ImGui::InputFloat(
    "Dolly Mouse Speed", &scene.orbit_dolly_move_camera.props_.dolly_speed_);
  ImGui::InputFloat(
    "Dolly Wheel Speed", &scene.orbit_dolly_wheel_camera.props_.dolly_speed_);
  ImGui::PopItemWidth();
  ImGui::Checkbox(
    "Pan Invert X", &scene.first_person_pan_camera.props_.pan_invert_x_);
  ImGui::Checkbox(
    "Pan Invert Y", &scene.first_person_pan_camera.props_.pan_invert_y_);
  ImGui::Text("Yaw Camera: ");
  ImGui::SameLine(100);
  ImGui::Text("%f", as::degrees(scene.camera.yaw));
  ImGui::End();

  const auto freq = double(bx::getHPFrequency());
  int64_t time_window = fps::calculateWindow(scene.fps, bx::getHPCounter());
  const double framerate = time_window > -1 ? (double)(scene.fps.MaxSamples - 1)
                                                / (double(time_window) / freq)
                                            : 0.0;
  const auto now = bx::getHPCounter();
  const auto delta = now - scene.prev;
  scene.prev = now;

  const float delta_time = delta / static_cast<float>(freq);

  as::mat4 camera_view;
  if (scene.camera_mode == CameraMode::Control) {
    scene.target_camera =
      scene.camera_system.stepCamera(scene.target_camera, delta_time);
    scene.camera = asci::smoothCamera(
      scene.camera, scene.target_camera, scene.smooth_props, delta_time);
    camera_view = as::mat4_from_affine(scene.camera.view());
  } else if (scene.camera_mode == CameraMode::Animation) {
    auto camera_transform_current = as::rigid(
      as::quat_slerp(
        scene.camera_transform_start.rotation,
        scene.camera_transform_end.rotation,
        as::smoother_step(scene.camera_animation_t)),
      as::vec_mix(
        scene.camera_transform_start.translation,
        scene.camera_transform_end.translation,
        as::smoother_step(scene.camera_animation_t)));

    camera_view =
      as::mat4_from_rigid(as::rigid_inverse(camera_transform_current));

    auto angles =
      eulerAngles(as::mat3_from_quat(camera_transform_current.rotation));
    scene.camera.pitch = std::get<0>(angles);
    scene.camera.yaw = std::get<1>(angles);
    scene.camera.look_at = camera_transform_current.translation;
    scene.target_camera = scene.camera;

    if (scene.camera_animation_t >= 1.0f) {
      scene.camera_mode = CameraMode::Control;
    }

    scene.camera_animation_t =
      as::clamp(scene.camera_animation_t + (delta_time / 1.0f), 0.0f, 1.0f);
  }

  float view[16];
  as::mat_to_arr(camera_view, view);

  float proj_p[16];
  as::mat_to_arr(scene.perspective_projection, proj_p);
  bgfx::setViewTransform(scene.main_view, view, proj_p);

  const float smooth_stop_start_mix = as::mix(
    nlt::smoothStart2(scene.debug.smooth_stop_start_mix_t),
    nlt::smoothStop2(scene.debug.smooth_stop_start_mix_t),
    scene.debug.smooth_stop_start_mix_t);
  const float smoother_stop_start_mix = as::mix(
    nlt::smoothStart3(scene.debug.smooth_stop_start_mix_t),
    nlt::smoothStop3(scene.debug.smooth_stop_start_mix_t),
    as::smooth_step(scene.debug.smooth_stop_start_mix_t));
  const float bezier3_1d =
    nlt::bezier3(
      as::vec3::zero(), as::vec3::axis_x(1.0f), as::vec3::axis_x(0.0f),
      as::vec3::axis_x(1.0f), scene.debug.smooth_stop_start_mix_t)
      .x;
  const float bezier5_1d =
    nlt::bezier5(
      as::vec3::zero(), as::vec3::axis_x(1.0f), as::vec3::axis_x(0.0f),
      as::vec3::axis_x(0.0f), as::vec3::axis_x(1.0f), as::vec3::axis_x(1.0f),
      scene.debug.smooth_stop_start_mix_t)
      .x;

  ImGui::Begin("Curves");
  ImGui::Checkbox("Linear", &scene.debug.linear);
  ImGui::Checkbox("Smooth Step", &scene.debug.smooth_step);
  ImGui::Checkbox("Smoother Step", &scene.debug.smoother_step);
  ImGui::Checkbox(
    "Smooth Stop Start Mix 2", &scene.debug.smooth_stop_start_mix2);
  ImGui::Checkbox(
    "Smooth Stop Start Mix 3", &scene.debug.smooth_stop_start_mix3);
  ImGui::SliderFloat("t", &scene.debug.smooth_stop_start_mix_t, 0.0f, 1.0f);
  ImGui::Text("Smooth Step: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", as::smooth_step(scene.debug.smooth_stop_start_mix_t));
  ImGui::Text("Smooth Stop Start Mix 2: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", smooth_stop_start_mix);
  ImGui::Text("Bezier3 1d: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", bezier3_1d);
  ImGui::Text("Smoother Step: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", as::smoother_step(scene.debug.smooth_stop_start_mix_t));
  ImGui::Text("Smooth Stop Start Mix 3: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", smoother_stop_start_mix);
  ImGui::Text("Bezier5 1d: ");
  ImGui::SameLine(180);
  ImGui::Text("%f", bezier5_1d);
  ImGui::Checkbox("Smooth Start 2", &scene.debug.smooth_start2);
  ImGui::Checkbox("Smooth Start 3", &scene.debug.smooth_start3);
  ImGui::Checkbox("Smooth Start 4", &scene.debug.smooth_start4);
  ImGui::Checkbox("Smooth Start 5", &scene.debug.smooth_start5);
  ImGui::Checkbox("Smooth Stop 2", &scene.debug.smooth_stop2);
  ImGui::Checkbox("Smooth Stop 3", &scene.debug.smooth_stop3);
  ImGui::Checkbox("Smooth Stop 4", &scene.debug.smooth_stop4);
  ImGui::Checkbox("Smooth Stop 5", &scene.debug.smooth_stop5);
  ImGui::Checkbox("Arch 2", &scene.debug.arch2);
  ImGui::Checkbox("Smooth Start Arch 3", &scene.debug.smooth_start_arch3);
  ImGui::Checkbox("Smooth Stop Arch 3", &scene.debug.smooth_stop_arch3);
  ImGui::Checkbox("Smooth Step Arch 4", &scene.debug.smooth_step_arch4);
  ImGui::Checkbox("Normalized Bezier 2", &scene.debug.normalized_bezier2);
  ImGui::Checkbox("Bezier Smooth Step", &scene.debug.bezier_smooth_step);
  ImGui::Checkbox("Normalized Bezier 3", &scene.debug.normalized_bezier3);
  ImGui::Checkbox("Normalized Bezier 4", &scene.debug.normalized_bezier4);
  ImGui::Checkbox("Normalized Bezier 5", &scene.debug.normalized_bezier5);
  ImGui::SliderFloat("b", &scene.debug.normalized_bezier_b, 0.0f, 1.0f);
  ImGui::SliderFloat("c", &scene.debug.normalized_bezier_c, 0.0f, 1.0f);
  ImGui::SliderFloat("d", &scene.debug.normalized_bezier_d, 0.0f, 1.0f);
  ImGui::SliderFloat("e", &scene.debug.normalized_bezier_e, 0.0f, 1.0f);

  auto debug_lines =
    dbg::DebugLines(scene.main_view, scene.simple_program.handle());

  // draw alignment transform
  drawTransform(debug_lines, as::affine_from_rigid(scene.camera_transform_end));

  // grid
  const auto grid_scale = 10.0f;
  const auto grid_camera_offset =
    as::vec_snap(scene.camera.look_at, grid_scale);
  const auto grid_dimension = 20;
  const auto grid_size = static_cast<float>(grid_dimension) * grid_scale;
  const auto grid_offset = grid_size * 0.5f;
  namespace ei = easy_iterator;
  for (auto line : ei::range(grid_dimension + 1)) {
    const auto start = (static_cast<float>(line) * grid_scale) - grid_offset;
    const auto flattened_offset =
      as::vec3(grid_camera_offset.x, 0.0f, grid_camera_offset.z);
    debug_lines.addLine(
      as::vec3(-grid_offset, 0.0f, start) + flattened_offset,
      as::vec3(0.0f + grid_offset, 0.0f, start) + flattened_offset, 0xff000000);
    debug_lines.addLine(
      as::vec3(start, 0.0f, -grid_offset) + flattened_offset,
      as::vec3(start, 0.0f, grid_offset) + flattened_offset, 0xff000000);
  }

  // draw camera look at
  if (!as::real_near(scene.camera.look_dist, 0.0f, 0.01f)) {
    float alpha = as::max(scene.camera.look_dist, -5.0f) / -5.0f;
    drawTransform(
      debug_lines, as::affine_from_vec3(scene.camera.look_at), alpha);
    debug_spheres.addSphere(
      as::mat4_from_mat3_vec3(as::mat3::identity(), scene.camera.look_at),
      as::vec4(as::vec3::zero(), alpha));
  }

  const auto p0 = scene.curve_handles.getHandle(scene.p0_index);
  const auto p1 = scene.curve_handles.getHandle(scene.p1_index);
  const auto c0 = scene.curve_handles.getHandle(scene.c0_index);
  const auto c1 = scene.curve_handles.getHandle(scene.c1_index);
  const auto c2 = scene.curve_handles.getHandle(scene.c2_index);
  const auto c3 = scene.curve_handles.getHandle(scene.c3_index);

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
    debug_lines.addLine(points[order][i], points[order][i + 1], 0xffaaaaaa);
  }

  // control handles
  for (as::index index = 0; index < order + 2; ++index) {
    const auto translation = as::mat4_from_mat3_vec3(
      as::mat3::identity(), scene.curve_handles.getHandle(index));
    const auto scale =
      as::mat4_from_mat3(as::mat3_scale(dbg::CurveHandles::HandleRadius));

    debug_circles.addCircle(
      as::mat_mul(scale, translation), as::vec4(as::vec3::zero(), 1.0f));
  }

  const auto line_granularity = 50;
  const auto line_length = 20.0f;
  for (auto i = 0; i < line_granularity; ++i) {
    float begin = i / float(line_granularity);
    float end = float(i + 1) / float(line_granularity);

    float x_begin = begin * line_length;
    float x_end = end * line_length;

    const auto sample_curve =
      [line_length, begin, end, &debug_lines, x_begin,
       x_end](auto fn, const uint32_t col = 0xff000000) {
        debug_lines.addLine(
          as::vec3(x_begin, as::mix(0.0f, line_length, fn(begin)), 0.0f),
          as::vec3(x_end, as::mix(0.0f, line_length, fn(end)), 0.0f), col);
      };

    if (order == 0) {
      debug_lines.addLine(
        nlt::bezier1(p0, p1, begin), nlt::bezier1(p0, p1, end), 0xff000000);
    }

    if (order == 1) {
      debug_lines.addLine(
        nlt::bezier2(p0, p1, c0, begin), nlt::bezier2(p0, p1, c0, end),
        0xff000000);
    }

    if (order == 2) {
      debug_lines.addLine(
        nlt::bezier3(p0, p1, c0, c1, begin), nlt::bezier3(p0, p1, c0, c1, end),
        0xff000000);
    }

    if (order == 3) {
      debug_lines.addLine(
        nlt::bezier4(p0, p1, c0, c1, c2, begin),
        nlt::bezier4(p0, p1, c0, c1, c2, end), 0xff000000);
    }

    if (order == 4) {
      debug_lines.addLine(
        nlt::bezier5(p0, p1, c0, c1, c2, c3, begin),
        nlt::bezier5(p0, p1, c0, c1, c2, c3, end), 0xff000000);
    }

    if (scene.debug.linear) {
      sample_curve([](const float a) { return a; });
    }

    if (scene.debug.arch2) {
      sample_curve(nlt::arch2);
    }

    if (scene.debug.smooth_start_arch3) {
      sample_curve(nlt::smoothStartArch3);
    }

    if (scene.debug.smooth_stop_arch3) {
      sample_curve(nlt::smoothStopArch3);
    }

    if (scene.debug.smooth_step_arch4) {
      sample_curve(nlt::smoothStepArch4);
    }

    if (scene.debug.smooth_step) {
      sample_curve(as::smooth_step<float>);
    }

    if (scene.debug.smoother_step) {
      sample_curve(as::smoother_step<float>);
    }

    if (scene.debug.smooth_stop_start_mix2) {
      sample_curve(nlt::smoothStepMixed);
      sample_curve(
        [&scene](const float sample) {
          return nlt::smoothStepMixer(
            sample, scene.debug.smooth_stop_start_mix_t);
        },
        0xff00ff00);
    }

    if (scene.debug.smooth_stop_start_mix3) {
      sample_curve(nlt::smootherStepMixed);
      sample_curve(
        [&scene](const float sample) {
          return nlt::smootherStepMixer(
            sample, scene.debug.smooth_stop_start_mix_t);
        },
        0xff00ff00);
    }

    if (scene.debug.smooth_start2) {
      sample_curve(nlt::smoothStart2);
    }

    if (scene.debug.smooth_start3) {
      sample_curve(nlt::smoothStart3);
    }

    if (scene.debug.smooth_start4) {
      sample_curve(nlt::smoothStart4);
    }

    if (scene.debug.smooth_start5) {
      sample_curve(nlt::smoothStart5);
    }

    if (scene.debug.smooth_stop2) {
      sample_curve(nlt::smoothStop2);
    }

    if (scene.debug.smooth_stop3) {
      sample_curve(nlt::smoothStop3);
    }

    if (scene.debug.smooth_stop4) {
      sample_curve(nlt::smoothStop4);
    }

    if (scene.debug.smooth_stop5) {
      sample_curve(nlt::smoothStop5);
    }

    if (scene.debug.bezier_smooth_step) {
      sample_curve(nlt::bezierSmoothStep);
    }

    if (scene.debug.normalized_bezier2) {
      sample_curve(
        [&scene](const float sample) {
          return nlt::normalizedBezier2(
            scene.debug.normalized_bezier_b, sample);
        },
        0xff0000ff);
    }

    if (scene.debug.normalized_bezier3) {
      sample_curve(
        [&scene](const float sample) {
          return nlt::normalizedBezier3(
            scene.debug.normalized_bezier_b, scene.debug.normalized_bezier_c,
            sample);
        },
        0xff0000ff);
    }

    if (scene.debug.normalized_bezier4) {
      sample_curve(
        [&scene](const float sample) {
          return nlt::normalizedBezier4(
            scene.debug.normalized_bezier_b, scene.debug.normalized_bezier_c,
            scene.debug.normalized_bezier_d, sample);
        },
        0xff0000ff);
    }

    if (scene.debug.normalized_bezier5) {
      sample_curve(
        [scene](const float sample) {
          return nlt::normalizedBezier5(
            scene.debug.normalized_bezier_b, scene.debug.normalized_bezier_c,
            scene.debug.normalized_bezier_d, scene.debug.normalized_bezier_e,
            sample);
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

  debug_lines.addLine(start, end, 0xff000000);

  // draw smooth line handles
  for (auto index :
       {scene.smooth_line_begin_index, scene.smooth_line_end_index}) {
    const auto translation = as::mat4_from_mat3_vec3(
      as::mat3::identity(), scene.curve_handles.getHandle(index));
    const auto scale =
      as::mat4_from_mat3(as::mat3_scale(dbg::CurveHandles::HandleRadius));

    debug_circles.addCircle(
      as::mat_mul(scale, translation), as::vec4(as::vec3::zero(), 1.0f));
  }

  // draw smooth line
  const auto smooth_line_begin =
    scene.curve_handles.getHandle(scene.smooth_line_begin_index);
  const auto smooth_line_end =
    scene.curve_handles.getHandle(scene.smooth_line_end_index);
  auto smooth_line =
    dbg::SmoothLine(scene.main_view, scene.simple_program.handle());
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

  const auto horizontal_position =
    as::vec_mix(start, end, interpolations[item](t));

  const auto translation =
    as::mat4_from_mat3_vec3(as::mat3::identity(), horizontal_position);
  const auto scale =
    as::mat4_from_mat3(as::mat3_scale(dbg::CurveHandles::HandleRadius));

  debug_circles.addCircle(
    as::mat_mul(scale, translation), as::vec4(as::vec3::zero(), 1.0f));

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

  debug_circles.addCircle(
    as::mat_mul(
      as::mat4_from_mat3(as::mat3_scale(dbg::CurveHandles::HandleRadius)),
      as::mat4_from_mat3_vec3(as::mat3::identity(), curve_position)),
    as::vec4(as::vec3::zero(), 1.0f));
  // animation end

  // screen space drawing
  float view_o[16];
  as::mat_to_arr(as::mat4::identity(), view_o);

  const as::mat4 orthographic_projection = as::ortho_d3d_lh(
    0.0f, scene.screen_dimension.x, scene.screen_dimension.y, 0.0f, 0.0f, 1.0f);

  float proj_o[16];
  as::mat_to_arr(orthographic_projection, proj_o);
  bgfx::setViewTransform(scene.ortho_view, view_o, proj_o);

  const auto start_screen = as::world_to_screen(
    start, scene.perspective_projection, scene.camera.view(),
    scene.screen_dimension);
  const auto end_screen = as::world_to_screen(
    end, scene.perspective_projection, scene.camera.view(),
    scene.screen_dimension);

  dbg::DebugLines dl_screen(scene.ortho_view, scene.simple_program.handle());
  dl_screen.addLine(
    as::vec3(start_screen.x, start_screen.y - 10.0f, 0.0f),
    as::vec3(start_screen.x, start_screen.y + 10.0f, 0.0f), 0xff000000);
  dl_screen.addLine(
    as::vec3(end_screen.x, end_screen.y - 10.0f, 0.0f),
    as::vec3(end_screen.x, end_screen.y + 10.0f, 0.0f), 0xff000000);
  dl_screen.submit();

  ImGui::Text("Framerate: ");
  ImGui::SameLine(100);
  ImGui::Text("%f", framerate);
  ImGui::End();

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
    debug_lines.addLine(
      as::vec3(offset, -10.0f, 0.0f),
      as::vec3(offset, -10.0f + ns::noise1dZeroToOne(i), 0.0f), 0xff000000);
    debug_lines.addLine(
      as::vec3(offset, -12.0f, 0.0f),
      as::vec3(offset, -12.0f + ns::noise1dMinusOneToOne(i), 0.0f), 0xff000000);
  }

  // draw perlin noise
  for (int64_t i = 0; i < 320; ++i) {
    const float offset = (i * 0.05f) + 2.0f;
    const float next_offset = ((float(i + 1)) * 0.05f) + 2.0f;
    debug_lines.addLine(
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

  const size_t quad_dimension = 100;
  dbg::DebugQuads debug_quads(scene.main_view, scene.instance_program.handle());
  debug_quads.reserveQuads(quad_dimension * quad_dimension);

  const auto noise_position = as::vec2_from_arr(noise2d_position);
  const auto starting_offset = as::vec3::axis_x(-12.0f);
  for (size_t r = 0; r < 100; ++r) {
    for (size_t c = 0; c < 100; ++c) {
      const as::vec2 p = as::vec2(float(c) * 0.1f, float(r) * 0.1f);
      const float grey =
        ns::perlinNoise2d((p + noise_position) * noise2d_freq, noise2d_offset)
          * noise2d_amp
        + 0.5f;

      if (draw_gradients && c % 10 == 0 && r % 10 == 0) {
        const as::vec2 p0 = as::vec_floor(p);
        const as::vec2 g0 = ns::gradient(ns::angle(p0, noise2d_offset));
        debug_lines.addLine(
          starting_offset + as::vec3(p0) / noise2d_freq,
          starting_offset + (as::vec3(p0) + as::vec3(g0)) / noise2d_freq,
          0xff000000);
      }

      const auto translation =
        as::mat4_from_vec3(as::vec3(p) + starting_offset);
      const auto scale = as::mat4_from_mat3(as::mat3_scale(0.1f));

      debug_quads.addQuad(
        as::mat_mul(scale, translation), as::vec4(as::vec3(grey), 1.0f));
    }
  }

  ImGui::Begin("Transforms");
  static float Translation[] = {-15.0f, 5.0f, 0.0f};
  ImGui::SliderFloat3("Translation", Translation, -50.0f, 50.0f);
  static float Rotation[] = {0.0f, 0.0f, 0.0f};
  ImGui::SliderFloat3("Rotation", Rotation, -360.0f, 360.0f);
  ImGui::End();

  dbg::DebugCubes debug_cubes(scene.main_view, scene.instance_program.handle());

  const as::vec3 start_position = as::vec3::zero();

  const as::rigid rigid_transformation(
    as::quat_rotation_zxy(
      as::radians(Rotation[0]), as::radians(Rotation[1]),
      as::radians(Rotation[2])),
    as::vec3_from_arr(Translation));
  const as::vec3 next_position_rigid =
    as::rigid_transform_pos(rigid_transformation, as::vec3::axis_z(0.5f));

  const as::affine affine_transformation(
    as::mat3_rotation_zxy(
      as::radians(Rotation[0]), as::radians(Rotation[1]),
      as::radians(Rotation[2])),
    as::vec3_from_arr(Translation));
  const as::vec3 next_position_affine =
    as::affine_transform_pos(affine_transformation, as::vec3::axis_z(0.5f));

  const as::rigid next_rigid =
    as::rigid_mul(as::rigid(as::quat::identity()), rigid_transformation);
  const as::affine next_affine =
    as::affine_mul(as::affine(as::mat3::identity()), affine_transformation);

  as::rigid another_rigid =
    as::rigid_mul(as::rigid(as::vec3::axis_z(-0.5f)), next_rigid);
  as::affine another_affine =
    as::affine_mul(as::affine(as::vec3::axis_z(-0.5f)), next_affine);

  debug_cubes.addCube(as::mat4_from_rigid(next_rigid), as::vec4::one());
  debug_cubes.addCube(as::mat4_from_affine(next_affine), as::vec4::axis_w());

  debug_spheres.addSphere(
    as::mat4_from_vec3(next_position_rigid)
      * as::mat4_from_mat3(as::mat3_scale(0.2f)),
    as::vec4::one());
  debug_spheres.addSphere(
    as::mat4_from_vec3(next_position_affine)
      * as::mat4_from_mat3(as::mat3_scale(0.2f)),
    as::vec4::axis_w());

  debug_spheres.addSphere(
    as::mat4_from_rigid(another_rigid)
      * as::mat4_from_mat3(as::mat3_scale(0.2f)),
    as::vec4::one());
  debug_spheres.addSphere(
    as::mat4_from_affine(another_affine)
      * as::mat4_from_mat3(as::mat3_scale(0.2f)),
    as::vec4::axis_w());

  debug_lines.submit();
  debug_quads.submit();
  debug_circles.submit();
  debug_cubes.submit();

  // include this in case nothing was submitted to draw
  bgfx::touch(scene.main_view);
  bgfx::touch(scene.ortho_view);
}

void teardown(transforms_scene_t& scene)
{
  scene.simple_program.deinit();
  scene.instance_program.deinit();
}
