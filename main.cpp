#include "1d-nonlinear-transformations.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include "as-camera-input/as-camera-input.hpp"
#include "as/as-math-ops.hpp"
#include "as/as-view.hpp"
#include "bgfx-helpers.h"
#include "bgfx-imgui/imgui_impl_bgfx.h"
#include "bgfx/bgfx.h"
#include "bgfx/platform.h"
#include "bx/math.h"
#include "bx/timer.h"
#include "curve-handles.h"
#include "easy_iterator.h"
#include "file-ops.h"
#include "fps.h"
#include "hierarchy-imgui.h"
#include "hsv-rgb.h"
#include "imgui.h"
#include "marching-cube-scene.h"
#include "marching-cubes/marching-cubes.h"
#include "noise.h"
#include "sdl-imgui/imgui_impl_sdl.h"
#include "smooth-line.h"
#include "thh-bgfx-debug/debug-cube.hpp"
#include "thh-bgfx-debug/debug-line.hpp"
#include "thh-bgfx-debug/debug-quad.hpp"
#include "thh-bgfx-debug/debug-shader.hpp"
#include "thh-bgfx-debug/debug-sphere.hpp"

#include <optional>
#include <tuple>

namespace asc
{

Handedness handedness()
{
  return Handedness::Left;
}

} // namespace asc

asci::MouseButton mouseFromSdl(const SDL_MouseButtonEvent* event)
{
  switch (event->button) {
    case SDL_BUTTON_LEFT:
      return asci::MouseButton::Left;
    case SDL_BUTTON_RIGHT:
      return asci::MouseButton::Right;
    case SDL_BUTTON_MIDDLE:
      return asci::MouseButton::Middle;
    default:
      return asci::MouseButton::Nil;
  }
}

asci::KeyboardButton keyboardFromSdl(const int key)
{
  switch (key) {
    case SDL_SCANCODE_W:
      return asci::KeyboardButton::W;
    case SDL_SCANCODE_S:
      return asci::KeyboardButton::S;
    case SDL_SCANCODE_A:
      return asci::KeyboardButton::A;
    case SDL_SCANCODE_D:
      return asci::KeyboardButton::D;
    case SDL_SCANCODE_Q:
      return asci::KeyboardButton::Q;
    case SDL_SCANCODE_E:
      return asci::KeyboardButton::E;
    case SDL_SCANCODE_LALT:
      return asci::KeyboardButton::LAlt;
    case SDL_SCANCODE_LSHIFT:
      return asci::KeyboardButton::LShift;
    case SDL_SCANCODE_LCTRL:
      return asci::KeyboardButton::Ctrl;
    default:
      return asci::KeyboardButton::Nil;
  }
}

asci::InputEvent sdlToInput(const SDL_Event* event)
{
  switch (event->type) {
    case SDL_MOUSEMOTION: {
      const auto* mouse_motion_event = (SDL_MouseMotionEvent*)event;
      return asci::CursorMotionEvent{
        {mouse_motion_event->x, mouse_motion_event->y}};
    }
    case SDL_MOUSEWHEEL: {
      const auto* mouse_wheel_event = (SDL_MouseWheelEvent*)event;
      return asci::ScrollEvent{mouse_wheel_event->y};
    }
    case SDL_MOUSEBUTTONDOWN: {
      const auto* mouse_event = (SDL_MouseButtonEvent*)event;
      return asci::MouseButtonEvent{
        mouseFromSdl(mouse_event), asci::ButtonAction::Down};
    }
    case SDL_MOUSEBUTTONUP: {
      const auto* mouse_event = (SDL_MouseButtonEvent*)event;
      return asci::MouseButtonEvent{
        mouseFromSdl(mouse_event), asci::ButtonAction::Up};
    }
    case SDL_KEYDOWN: {
      const auto* keyboard_event = (SDL_KeyboardEvent*)event;
      return asci::KeyboardButtonEvent{
        keyboardFromSdl(keyboard_event->keysym.scancode),
        asci::ButtonAction::Down, event->key.repeat != 0u};
    }
    case SDL_KEYUP: {
      const auto* keyboard_event = (SDL_KeyboardEvent*)event;
      return asci::KeyboardButtonEvent{
        keyboardFromSdl(keyboard_event->keysym.scancode),
        asci::ButtonAction::Up, event->key.repeat != 0u};
    }
    default:
      return std::monostate{};
  }
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

int main(int argc, char** argv)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  const int width = 1024;
  const int height = 768;
  const float aspect = float(width) / float(height);
  SDL_Window* window = SDL_CreateWindow(
    argv[0], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
    SDL_WINDOW_SHOWN);

  if (window == nullptr) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (SDL_GetWindowWMInfo(window, &wmi) == 0) {
    return 1;
  }

  bgfx::renderFrame(); // single threaded mode

  bgfx::PlatformData pd{};
#if BX_PLATFORM_WINDOWS
  pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_OSX
  pd.nwh = wmi.info.cocoa.window;
#elif BX_PLATFORM_LINUX
  pd.ndt = wmi.info.x11.display;
  pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX

  bgfx::Init bgfx_init;
  bgfx_init.type = bgfx::RendererType::Count; // auto choose renderer
  bgfx_init.resolution.width = width;
  bgfx_init.resolution.height = height;
  bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
  bgfx_init.platformData = pd;
  bgfx::init(bgfx_init);

  dbg::DebugVertex::init();
  dbg::DebugCircles::init();
  dbg::DebugCubes::init();

  const auto screen_dimension = as::vec2i(width, height);
  const bgfx::ViewId main_view = 0;
  const bgfx::ViewId ortho_view = 1;

  // cornflower clear color
  bgfx::setViewClear(
    main_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
  bgfx::setViewRect(main_view, 0, 0, width, height);
  bgfx::setViewClear(ortho_view, BGFX_CLEAR_DEPTH);
  bgfx::setViewRect(ortho_view, 0, 0, width, height);
  // dummy draw call to make sure main_view is cleared if no other draw calls
  // are submitted
  bgfx::touch(main_view);
  bgfx::touch(ortho_view);

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  ImGui_Implbgfx_Init(255);
#if BX_PLATFORM_WINDOWS
  ImGui_ImplSDL2_InitForD3D(window);
#elif BX_PLATFORM_OSX
  ImGui_ImplSDL2_InitForMetal(window);
#elif BX_PLATFORM_LINUX
  ImGui_ImplSDL2_InitForOpenGL(window, nullptr);
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX

  dbg::EmbeddedShaderProgram simple_program;
  simple_program.init(dbg::SimpleEmbeddedShaderArgs);

  dbg::EmbeddedShaderProgram instance_program;
  instance_program.init(dbg::InstanceEmbeddedShaderArgs);

  asci::SmoothProps smooth_props{};
  asc::Camera camera{};
  // initial camera position and orientation
  camera.look_at = as::vec3(24.3f, 1.74f, -33.0f);

  asc::Camera target_camera = camera;

  const float fov = as::radians(60.0f);
  const as::mat4 perspective_projection =
    as::perspective_d3d_lh(fov, float(width) / float(height), 0.01f, 1000.0f);

  // debug settings
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

  dbg::CurveHandles curve_handles;
  const auto p0_index = curve_handles.addHandle(as::vec3(2.0f, -8.0f, 0.0f));
  const auto p1_index = curve_handles.addHandle(as::vec3(18.0f, -8.0f, 0.0f));
  const auto c0_index = curve_handles.addHandle(as::vec3(5.2f, -4.0f, 0.0f));
  const auto c1_index = curve_handles.addHandle(as::vec3(8.4f, -4.0f, 0.0f));
  const auto c2_index = curve_handles.addHandle(as::vec3(11.6f, -4.0f, 0.0f));
  const auto c3_index = curve_handles.addHandle(as::vec3(14.8f, -4.0f, 0.0f));

  const auto smooth_line_begin_index =
    curve_handles.addHandle(as::vec3::axis_x(22.0f));
  const auto smooth_line_end_index =
    curve_handles.addHandle(as::vec3(25.0f, 4.0f, 0.0f));

  auto prev = bx::getHPCounter();

  auto first_person_rotate_camera =
    asci::RotateCameraInput{asci::MouseButton::Right};
  auto first_person_pan_camera = asci::PanCameraInput{asci::lookPan};
  auto first_person_translate_camera =
    asci::TranslateCameraInput{asci::lookTranslation};
  auto first_person_wheel_camera = asci::ScrollTranslationCameraInput{};

  auto orbit_camera = asci::OrbitCameraInput{};
  auto orbit_rotate_camera = asci::RotateCameraInput{asci::MouseButton::Left};
  auto orbit_translate_camera =
    asci::TranslateCameraInput{asci::orbitTranslation};
  auto orbit_dolly_wheel_camera = asci::OrbitDollyScrollCameraInput{};
  auto orbit_dolly_move_camera = asci::OrbitDollyCursorMoveCameraInput{};
  auto orbit_pan_camera = asci::PanCameraInput{asci::orbitPan};
  orbit_camera.orbit_cameras_.addCamera(&orbit_rotate_camera);
  orbit_camera.orbit_cameras_.addCamera(&orbit_translate_camera);
  orbit_camera.orbit_cameras_.addCamera(&orbit_dolly_wheel_camera);
  orbit_camera.orbit_cameras_.addCamera(&orbit_dolly_move_camera);
  orbit_camera.orbit_cameras_.addCamera(&orbit_pan_camera);

  asci::Cameras cameras;
  cameras.addCamera(&first_person_rotate_camera);
  cameras.addCamera(&first_person_pan_camera);
  cameras.addCamera(&first_person_translate_camera);
  cameras.addCamera(&first_person_wheel_camera);
  cameras.addCamera(&orbit_camera);

  asci::CameraSystem camera_system;
  camera_system.cameras_ = cameras;

  as::rigid camera_transform_start = as::rigid(as::quat::identity());
  as::rigid camera_transform_end = as::rigid(as::quat::identity());

  enum class CameraMode
  {
    Control,
    Animation
  };

  float camera_animation_t = 0.0f;
  CameraMode camera_mode = CameraMode::Control;

  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_sample_entities(entities);

  hy::interaction_t interaction;
  interaction.select(root_handles.front(), entities, root_handles);

  fps::Fps fps;
  for (bool quit = false; !quit;) {
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
      as::vec2i(x, y), perspective_projection, camera.view(), screen_dimension);
    const auto ray_origin = camera.transform().translation;
    const auto ray_direction = as::vec_normalize(world_position - ray_origin);
    const auto hit_distance =
      intersectPlane(ray_origin, ray_direction, as::vec4(as::vec3::axis_z()));

    SDL_Event current_event;
    while (SDL_PollEvent(&current_event) != 0) {
      camera_system.handleEvents(sdlToInput(&current_event));
      ImGui_ImplSDL2_ProcessEvent(&current_event);
      if (current_event.type == SDL_QUIT) {
        quit = true;
        break;
      }
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
      }
    }

    dbg::DebugCircles debug_circles(main_view, instance_program.handle());
    dbg::DebugSpheres debug_spheres(debug_circles);

    if (curve_handles.dragging() && hit_distance > 0.0f) {
      const auto next_hit = ray_origin + ray_direction * hit_distance;
      curve_handles.updateDrag(next_hit);
    }

    ImGui_Implbgfx_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);

    ImGui::NewFrame();

    ImGui::Begin("Camera");
    ImGui::PushItemWidth(70);
    ImGui::InputFloat(
      "Free Look Rotate Speed",
      &first_person_rotate_camera.props_.rotate_speed_);
    ImGui::InputFloat(
      "Free Look Pan Speed", &first_person_pan_camera.props_.pan_speed_);
    ImGui::InputFloat(
      "Translate Speed",
      &first_person_translate_camera.props_.translate_speed_);
    ImGui::InputFloat("Look Smoothness", &smooth_props.look_smoothness_);
    ImGui::InputFloat("Move Smoothness", &smooth_props.move_smoothness_);
    ImGui::InputFloat(
      "Boost Multiplier",
      &first_person_translate_camera.props_.boost_multiplier_);
    ImGui::InputFloat(
      "Default Orbit Distance", &orbit_camera.props_.default_orbit_distance_);
    ImGui::InputFloat(
      "Max Orbit Distance", &orbit_camera.props_.max_orbit_distance_);
    ImGui::InputFloat("Orbit Speed", &orbit_rotate_camera.props_.rotate_speed_);
    ImGui::InputFloat(
      "Dolly Mouse Speed", &orbit_dolly_move_camera.props_.dolly_speed_);
    ImGui::InputFloat(
      "Dolly Wheel Speed", &orbit_dolly_wheel_camera.props_.dolly_speed_);
    ImGui::PopItemWidth();
    ImGui::Checkbox(
      "Pan Invert X", &first_person_pan_camera.props_.pan_invert_x_);
    ImGui::Checkbox(
      "Pan Invert Y", &first_person_pan_camera.props_.pan_invert_y_);
    ImGui::Text("Yaw Camera: ");
    ImGui::SameLine(100);
    ImGui::Text("%f", as::degrees(camera.yaw));
    ImGui::End();

    const auto freq = double(bx::getHPFrequency());
    int64_t time_window = fps::calculateWindow(fps, bx::getHPCounter());
    const double framerate = time_window > -1 ? (double)(fps.MaxSamples - 1)
                                                  / (double(time_window) / freq)
                                              : 0.0;
    const auto now = bx::getHPCounter();
    const auto delta = now - prev;
    prev = now;

    const float delta_time = delta / static_cast<float>(freq);

    as::mat4 camera_view;
    if (camera_mode == CameraMode::Control) {
      target_camera = camera_system.stepCamera(target_camera, delta_time);
      camera =
        asci::smoothCamera(camera, target_camera, smooth_props, delta_time);
      camera_view = as::mat4_from_affine(camera.view());
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

      auto angles =
        eulerAngles(as::mat3_from_quat(camera_transform_current.rotation));
      camera.pitch = std::get<0>(angles);
      camera.yaw = std::get<1>(angles);
      camera.look_at = camera_transform_current.translation;
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
    bgfx::setViewTransform(main_view, view, proj_p);

    const float smooth_stop_start_mix = as::mix(
      nlt::smoothStart2(smooth_stop_start_mix_t),
      nlt::smoothStop2(smooth_stop_start_mix_t), smooth_stop_start_mix_t);
    const float smoother_stop_start_mix = as::mix(
      nlt::smoothStart3(smooth_stop_start_mix_t),
      nlt::smoothStop3(smooth_stop_start_mix_t),
      as::smooth_step(smooth_stop_start_mix_t));
    const float bezier3_1d =
      nlt::bezier3(
        as::vec3::zero(), as::vec3::axis_x(1.0f), as::vec3::axis_x(0.0f),
        as::vec3::axis_x(1.0f), smooth_stop_start_mix_t)
        .x;
    const float bezier5_1d =
      nlt::bezier5(
        as::vec3::zero(), as::vec3::axis_x(1.0f), as::vec3::axis_x(0.0f),
        as::vec3::axis_x(0.0f), as::vec3::axis_x(1.0f), as::vec3::axis_x(1.0f),
        smooth_stop_start_mix_t)
        .x;

    ImGui::Begin("Curves");
    ImGui::Checkbox("Linear", &linear);
    ImGui::Checkbox("Smooth Step", &smooth_step);
    ImGui::Checkbox("Smoother Step", &smoother_step);
    ImGui::Checkbox("Smooth Stop Start Mix 2", &smooth_stop_start_mix2);
    ImGui::Checkbox("Smooth Stop Start Mix 3", &smooth_stop_start_mix3);
    ImGui::SliderFloat("t", &smooth_stop_start_mix_t, 0.0f, 1.0f);
    ImGui::Text("Smooth Step: ");
    ImGui::SameLine(180);
    ImGui::Text("%f", as::smooth_step(smooth_stop_start_mix_t));
    ImGui::Text("Smooth Stop Start Mix 2: ");
    ImGui::SameLine(180);
    ImGui::Text("%f", smooth_stop_start_mix);
    ImGui::Text("Bezier3 1d: ");
    ImGui::SameLine(180);
    ImGui::Text("%f", bezier3_1d);
    ImGui::Text("Smoother Step: ");
    ImGui::SameLine(180);
    ImGui::Text("%f", as::smoother_step(smooth_stop_start_mix_t));
    ImGui::Text("Smooth Stop Start Mix 3: ");
    ImGui::SameLine(180);
    ImGui::Text("%f", smoother_stop_start_mix);
    ImGui::Text("Bezier5 1d: ");
    ImGui::SameLine(180);
    ImGui::Text("%f", bezier5_1d);
    ImGui::Checkbox("Smooth Start 2", &smooth_start2);
    ImGui::Checkbox("Smooth Start 3", &smooth_start3);
    ImGui::Checkbox("Smooth Start 4", &smooth_start4);
    ImGui::Checkbox("Smooth Start 5", &smooth_start5);
    ImGui::Checkbox("Smooth Stop 2", &smooth_stop2);
    ImGui::Checkbox("Smooth Stop 3", &smooth_stop3);
    ImGui::Checkbox("Smooth Stop 4", &smooth_stop4);
    ImGui::Checkbox("Smooth Stop 5", &smooth_stop5);
    ImGui::Checkbox("Arch 2", &arch2);
    ImGui::Checkbox("Smooth Start Arch 3", &smooth_start_arch3);
    ImGui::Checkbox("Smooth Stop Arch 3", &smooth_stop_arch3);
    ImGui::Checkbox("Smooth Step Arch 4", &smooth_step_arch4);
    ImGui::Checkbox("Bezier Smooth Step", &bezier_smooth_step);
    ImGui::Checkbox("Normalized Bezier 2", &normalized_bezier2);
    ImGui::Checkbox("Normalized Bezier 3", &normalized_bezier3);
    ImGui::Checkbox("Normalized Bezier 4", &normalized_bezier4);
    ImGui::Checkbox("Normalized Bezier 5", &normalized_bezier5);
    ImGui::SliderFloat("b", &normalized_bezier_b, 0.0f, 1.0f);
    ImGui::SliderFloat("c", &normalized_bezier_c, 0.0f, 1.0f);
    ImGui::SliderFloat("d", &normalized_bezier_d, 0.0f, 1.0f);
    ImGui::SliderFloat("e", &normalized_bezier_e, 0.0f, 1.0f);

    auto debug_lines = dbg::DebugLines(main_view, simple_program.handle());

    // draw alignment transform
    drawTransform(debug_lines, as::affine_from_rigid(camera_transform_end));

    // grid
    const auto grid_scale = 10.0f;
    const auto grid_camera_offset = as::vec_snap(camera.look_at, grid_scale);
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
        as::vec3(0.0f + grid_offset, 0.0f, start) + flattened_offset,
        0xff000000);
      debug_lines.addLine(
        as::vec3(start, 0.0f, -grid_offset) + flattened_offset,
        as::vec3(start, 0.0f, grid_offset) + flattened_offset, 0xff000000);
    }

    // draw camera look at
    if (!as::real_near(camera.look_dist, 0.0f, 0.01f)) {
      float alpha = as::max(camera.look_dist, -5.0f) / -5.0f;
      drawTransform(debug_lines, as::affine_from_vec3(camera.look_at), alpha);
      debug_spheres.addSphere(
        as::mat4_from_mat3_vec3(as::mat3::identity(), camera.look_at),
        as::vec4(as::vec3::zero(), alpha));
    }

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
    static const char* orders[] = {
      "First", "Second", "Third", "Fourth", "Fifth"};

    ImGui::Combo("Curve Order", &order, orders, std::size(orders));

    // control lines
    for (as::index i = 0; i < points[order].size(); i += 2) {
      debug_lines.addLine(points[order][i], points[order][i + 1], 0xffaaaaaa);
    }

    // control handles
    for (as::index index = 0; index < order + 2; ++index) {
      const auto translation = as::mat4_from_mat3_vec3(
        as::mat3::identity(), curve_handles.getHandle(index));
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
          nlt::bezier3(p0, p1, c0, c1, begin),
          nlt::bezier3(p0, p1, c0, c1, end), 0xff000000);
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

      if (linear) {
        sample_curve([](const float a) { return a; });
      }

      if (arch2) {
        sample_curve(nlt::arch2);
      }

      if (smooth_start_arch3) {
        sample_curve(nlt::smoothStartArch3);
      }

      if (smooth_stop_arch3) {
        sample_curve(nlt::smoothStopArch3);
      }

      if (smooth_step_arch4) {
        sample_curve(nlt::smoothStepArch4);
      }

      if (smooth_step) {
        sample_curve(as::smooth_step<float>);
      }

      if (smoother_step) {
        sample_curve(as::smoother_step<float>);
      }

      if (smooth_stop_start_mix2) {
        sample_curve(nlt::smoothStepMixed);
        sample_curve(
          [smooth_stop_start_mix_t](const float sample) {
            return nlt::smoothStepMixer(sample, smooth_stop_start_mix_t);
          },
          0xff00ff00);
      }

      if (smooth_stop_start_mix3) {
        sample_curve(nlt::smootherStepMixed);
        sample_curve(
          [smooth_stop_start_mix_t](const float sample) {
            return nlt::smootherStepMixer(sample, smooth_stop_start_mix_t);
          },
          0xff00ff00);
      }

      if (smooth_start2) {
        sample_curve(nlt::smoothStart2);
      }

      if (smooth_start3) {
        sample_curve(nlt::smoothStart3);
      }

      if (smooth_start4) {
        sample_curve(nlt::smoothStart4);
      }

      if (smooth_start5) {
        sample_curve(nlt::smoothStart5);
      }

      if (smooth_stop2) {
        sample_curve(nlt::smoothStop2);
      }

      if (smooth_stop3) {
        sample_curve(nlt::smoothStop3);
      }

      if (smooth_stop4) {
        sample_curve(nlt::smoothStop4);
      }

      if (smooth_stop5) {
        sample_curve(nlt::smoothStop5);
      }

      if (bezier_smooth_step) {
        sample_curve(nlt::bezierSmoothStep);
      }

      if (normalized_bezier2) {
        sample_curve(
          [normalized_bezier_b](const float sample) {
            return nlt::normalizedBezier2(normalized_bezier_b, sample);
          },
          0xff0000ff);
      }

      if (normalized_bezier3) {
        sample_curve(
          [normalized_bezier_b, normalized_bezier_c](const float sample) {
            return nlt::normalizedBezier3(
              normalized_bezier_b, normalized_bezier_c, sample);
          },
          0xff0000ff);
      }

      if (normalized_bezier4) {
        sample_curve(
          [normalized_bezier_b, normalized_bezier_c,
           normalized_bezier_d](const float sample) {
            return nlt::normalizedBezier4(
              normalized_bezier_b, normalized_bezier_c, normalized_bezier_d,
              sample);
          },
          0xff0000ff);
      }

      if (normalized_bezier5) {
        sample_curve(
          [normalized_bezier_b, normalized_bezier_c, normalized_bezier_d,
           normalized_bezier_e](const float sample) {
            return nlt::normalizedBezier5(
              normalized_bezier_b, normalized_bezier_c, normalized_bezier_d,
              normalized_bezier_e, sample);
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
    for (auto index : {smooth_line_begin_index, smooth_line_end_index}) {
      const auto translation = as::mat4_from_mat3_vec3(
        as::mat3::identity(), curve_handles.getHandle(index));
      const auto scale =
        as::mat4_from_mat3(as::mat3_scale(dbg::CurveHandles::HandleRadius));

      debug_circles.addCircle(
        as::mat_mul(scale, translation), as::vec4(as::vec3::zero(), 1.0f));
    }

    // draw smooth line
    const auto smooth_line_begin =
      curve_handles.getHandle(smooth_line_begin_index);
    const auto smooth_line_end = curve_handles.getHandle(smooth_line_end_index);
    auto smooth_line = dbg::SmoothLine(main_view, simple_program.handle());
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

    const as::mat4 orthographic_projection =
      as::ortho_d3d_lh(0.0f, width, height, 0.0f, 0.0f, 1.0f);

    float proj_o[16];
    as::mat_to_arr(orthographic_projection, proj_o);
    bgfx::setViewTransform(ortho_view, view_o, proj_o);

    const auto start_screen = as::world_to_screen(
      start, perspective_projection, camera.view(), screen_dimension);
    const auto end_screen = as::world_to_screen(
      end, perspective_projection, camera.view(), screen_dimension);

    dbg::DebugLines dl_screen(ortho_view, simple_program.handle());
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
        as::vec3(offset, -12.0f + ns::noise1dMinusOneToOne(i), 0.0f),
        0xff000000);
    }

    // draw perlin noise
    for (int64_t i = 0; i < 320; ++i) {
      const float offset = (i * 0.05f) + 2.0f;
      const float next_offset = ((float(i + 1)) * 0.05f) + 2.0f;
      debug_lines.addLine(
        as::vec3(
          offset,
          -14.0f
            + ns::perlinNoise1d(offset * noise1_freq, noise1_offset)
                * noise1_amp
            + ns::perlinNoise1d(offset * noise2_freq, noise2_offset)
                * noise2_amp
            + ns::perlinNoise1d(offset * noise3_freq, noise3_offset)
                * noise3_amp,
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
    dbg::DebugQuads debug_quads(main_view, instance_program.handle());
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

    // imgui hierarchy experiments
    hy_ig::imgui_interaction_draw_list_hierarchy(
      entities, interaction, root_handles);
    hy_ig::imgui_interaction_normal_hierarchy(
      entities, interaction, root_handles);
    hy_ig::imgui_only_recursive_hierarchy(entities, root_handles);

    dbg::DebugCubes debug_cubes(main_view, instance_program.handle());

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
    bgfx::touch(main_view);
    bgfx::touch(ortho_view);

    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

    bgfx::frame();
  }

  simple_program.deinit();
  instance_program.deinit();

  ImGui_ImplSDL2_Shutdown();
  ImGui_Implbgfx_Shutdown();

  ImGui::DestroyContext();
  bgfx::shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
