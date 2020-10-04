#include "1d-nonlinear-transformations.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include "as-camera-sdl/as-camera-sdl.h"
#include "as-camera/as-camera-controller.hpp"
#include "as/as-math-ops.hpp"
#include "as/as-view.hpp"
#include "bgfx-imgui/imgui_impl_bgfx.h"
#include "bgfx/bgfx.h"
#include "bgfx/platform.h"
#include "bx/math.h"
#include "bx/timer.h"
#include "curve-handles.h"
#include "debug-circle.h"
#include "debug-lines.h"
#include "file-ops.h"
#include "fps.h"
#include "imgui.h"
#include "sdl-imgui/imgui_impl_sdl.h"

#include <optional>
#include <tuple>

static bgfx::ShaderHandle createShader(
  const std::string& shader, const char* name)
{
  const bgfx::Memory* mem = bgfx::copy(shader.data(), shader.size());
  const bgfx::ShaderHandle handle = bgfx::createShader(mem);
  bgfx::setName(handle, name);
  return handle;
}

std::optional<bgfx::ProgramHandle> createShaderProgram(
  const char* vert_shader_path, const char* frag_shader_path)
{
  std::string vshader;
  if (!fileops::read_file(vert_shader_path, vshader)) {
    return {};
  }

  std::string fshader;
  if (!fileops::read_file(frag_shader_path, fshader)) {
    return {};
  }

  bgfx::ShaderHandle vsh = createShader(vshader, "vshader");
  bgfx::ShaderHandle fsh = createShader(fshader, "fshader");

  return bgfx::createProgram(vsh, fsh, true);
}

as::vec2i worldToScreen(
  const as::vec3 worldPosition, const as::mat4& projection,
  const as::affine& view, const as::vec2i& screenDimension)
{
  const as::vec4 clip = projection * as::mat4_from_affine(view)
                      * as::vec4_from_vec3(worldPosition, 1.0f);
  const as::vec3 ndc = as::vec3_from_vec4(clip / clip.w);
  const as::vec2 screen = (as::vec2_from_vec3(ndc) + as::vec2(1.0f)) * 0.5f;
  return as::vec2i(
    static_cast<as::vec2i::value_type>(
      screen.x * static_cast<as::vec2::value_type>(screenDimension.x)),
    static_cast<as::vec2i::value_type>(
      (1.0f - screen.y)
      * static_cast<as::vec2::value_type>(screenDimension.y)));
}

as::vec3 screenToWorld(
  const as::vec2i screenPosition, const as::mat4& projection,
  const as::affine& view, const as::vec2i& screenDimension)
{
  const as::vec2 normalizedScreen = as::vec2(
    screenPosition.x / static_cast<as::vec2::value_type>(screenDimension.x),
    (screenDimension.y - screenPosition.y)
      / static_cast<as::vec2::value_type>(screenDimension.y));
  const as::vec2 ndc = (normalizedScreen * 2.0f) - as::vec2(1.0f);
  as::vec4 worldPosition = as::mat4_from_affine(as::affine_inverse(view))
                         * as::mat_inverse(projection)
                         * as::vec4(ndc.x, ndc.y, 0.0f, 1.0f);
  worldPosition /= worldPosition.w;
  return as::vec3_from_vec4(worldPosition);
}

float intersectPlane(
  const as::vec3& origin, const as::vec3& direction, const as::vec4& plane)
{
  return -(as::vec_dot(origin, as::vec3_from_vec4(plane)) + plane.w)
       / as::vec_dot(direction, as::vec3_from_vec4(plane));
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
  if (!SDL_GetWindowWMInfo(window, &wmi)) {
    return 1;
  }

  bgfx::renderFrame(); // single threaded mode

  bgfx::PlatformData pd{};
#if BX_PLATFORM_WINDOWS
  pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_OSX
  pd.nwh = wmi.info.cocoa.window;
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX

  bgfx::Init bgfx_init;
  bgfx_init.type = bgfx::RendererType::Count; // auto choose renderer
  bgfx_init.resolution.width = width;
  bgfx_init.resolution.height = height;
  bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
  bgfx_init.platformData = pd;
  bgfx::init(bgfx_init);

  dbg::DebugVertex::init();

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
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX

  const bgfx::ProgramHandle program_col =
    createShaderProgram(
      "shader/simple/v_simple.bin", "shader/simple/f_simple.bin")
      .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));

  asc::Camera camera{};
  // initial camera position and orientation
  camera.look_at = as::vec3(19.44f, 5.57f, -26.54f);

  // initial mouse state
  MouseState mouse_state = mouseState();

  // camera control structure
  asc::CameraControl camera_control{};
  camera_control.pitch = camera.pitch;
  camera_control.yaw = camera.yaw;

  // camera properties
  asc::CameraProperties camera_props{};
  camera_props.rotate_speed = 0.005f;
  camera_props.translate_speed = 10.0f;
  camera_props.look_smoothness = 5.0f;

  const as::mat4 perspective_projection = as::perspective_d3d_lh(
    as::radians(60.0f), float(width) / float(height), 0.01f, 100.0f);

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
  const auto p0_index = curve_handles.addHandle(as::vec3(5.0f, -8.0f, 0.0f));
  const auto p1_index = curve_handles.addHandle(as::vec3(15.0f, -8.0f, 0.0f));
  const auto c0_index = curve_handles.addHandle(as::vec3(7.0f, -3.0f, 0.0f));
  const auto c1_index = curve_handles.addHandle(as::vec3(9.0f, -3.0f, 0.0f));
  const auto c2_index = curve_handles.addHandle(as::vec3(11.0f, -3.0f, 0.0f));
  const auto c3_index = curve_handles.addHandle(as::vec3(13.0f, -3.0f, 0.0f));

  auto prev = bx::getHPCounter();

  fps::Fps fps;
  for (bool quit = false; !quit;) {

    int x, y;
    SDL_GetMouseState(&x, &y);
    const auto orientation = as::affine_inverse(camera.view()).rotation;
    const auto worldPosition = screenToWorld(
      as::vec2i(x, y), perspective_projection, camera.view(), screen_dimension);
    const auto rayOrigin = camera.look_at;
    const auto rayDirection = as::vec_normalize(worldPosition - rayOrigin);

    const auto hitDistance = intersectPlane(
      rayOrigin, rayDirection, as::vec4(as::vec3::axis_z(), 0.0f));

    SDL_Event current_event;
    while (SDL_PollEvent(&current_event) != 0) {
      updateCameraControlKeyboardSdl(
        current_event, camera_control, camera_props);
      ImGui_ImplSDL2_ProcessEvent(&current_event);
      if (current_event.type == SDL_QUIT) {
        quit = true;
        break;
      }

      if (current_event.type == SDL_MOUSEBUTTONDOWN) {
        if (hitDistance >= 0.0f) {
          const auto hit = rayOrigin + rayDirection * hitDistance;
          curve_handles.tryBeginDrag(hit);
        }
      }

      if (current_event.type == SDL_MOUSEBUTTONUP) {
        curve_handles.clearDrag();
      }
    }

    if (curve_handles.dragging() && hitDistance > 0.0f) {
      const auto nextHit = rayOrigin + rayDirection * hitDistance;
      curve_handles.updateDrag(nextHit);
    }

    updateCameraControlMouseSdl(camera_control, camera_props, mouse_state);

    ImGui_Implbgfx_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);

    ImGui::NewFrame();

    const auto freq = double(bx::getHPFrequency());
    int64_t time_window = fps::calculateWindow(fps, bx::getHPCounter());
    const double framerate = time_window > -1 ? (double)(fps.MaxSamples - 1)
                                                  / (double(time_window) / freq)
                                              : 0.0;
    const auto now = bx::getHPCounter();
    const auto delta = now - prev;
    prev = now;

    const float dt = delta / freq;

    asc::updateCamera(
      camera, camera_control, camera_props, dt, asc::Handedness::Left);

    float view[16];
    as::mat_to_arr(as::mat4_from_affine(camera.view()), view);

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
    ImGui::Checkbox("Bezier Smooth Step", &bezier_smooth_step);
    ImGui::Checkbox("Normalized Bezier 2", &normalized_bezier2);
    ImGui::Checkbox("Normalized Bezier 3", &normalized_bezier3);
    ImGui::Checkbox("Normalized Bezier 4", &normalized_bezier4);
    ImGui::Checkbox("Normalized Bezier 5", &normalized_bezier5);
    ImGui::SliderFloat("b", &normalized_bezier_b, 0.0f, 1.0f);
    ImGui::SliderFloat("c", &normalized_bezier_c, 0.0f, 1.0f);
    ImGui::SliderFloat("d", &normalized_bezier_d, 0.0f, 1.0f);
    ImGui::SliderFloat("e", &normalized_bezier_e, 0.0f, 1.0f);

    for (as::index index = 0; index < curve_handles.size(); ++index) {
      const auto circle = dbg::DebugCircle(
        as::mat4_from_mat3_vec3(
          as::mat3::identity(), curve_handles.getHandle(index)),
        dbg::CurveHandles::HandleRadius, main_view, program_col);
      circle.draw();
    }

    auto debugLinesGraph = dbg::DebugLines(main_view, program_col);

    const auto p0 = curve_handles.getHandle(p0_index);
    const auto p1 = curve_handles.getHandle(p1_index);
    const auto c0 = curve_handles.getHandle(c0_index);
    const auto c1 = curve_handles.getHandle(c1_index);
    const auto c2 = curve_handles.getHandle(c2_index);
    const auto c3 = curve_handles.getHandle(c3_index);

    // control lines
    debugLinesGraph.addLine(p0, c0, 0xffaaaaaa);
    debugLinesGraph.addLine(c0, c1, 0xffaaaaaa);
    debugLinesGraph.addLine(c1, c2, 0xffaaaaaa);
    debugLinesGraph.addLine(c2, c3, 0xffaaaaaa);
    debugLinesGraph.addLine(c3, p1, 0xffaaaaaa);

    const auto lineGranularity = 50;
    const auto lineLength = 20.0f;
    for (auto i = 0; i < lineGranularity; ++i) {
      float begin = i / float(lineGranularity);
      float end = (i + 1) / float(lineGranularity);

      float x_begin = begin * lineLength;
      float x_end = end * lineLength;

      // bezier1 (linear)
      debugLinesGraph.addLine(
        nlt::bezier1(p0, p1, begin), nlt::bezier1(p0, p1, end), 0xff000000);

      // bezier2
      debugLinesGraph.addLine(
        nlt::bezier2(p0, p1, c0, begin), nlt::bezier2(p0, p1, c0, end),
        0xff000000);

      // bezier3
      debugLinesGraph.addLine(
        nlt::bezier3(p0, p1, c0, c1, begin), nlt::bezier3(p0, p1, c0, c1, end),
        0xff000000);

      // bezier4
      debugLinesGraph.addLine(
        nlt::bezier4(p0, p1, c0, c1, c2, begin),
        nlt::bezier4(p0, p1, c0, c1, c2, end), 0xff000000);

      // bezier5
      debugLinesGraph.addLine(
        nlt::bezier5(p0, p1, c0, c1, c2, c3, begin),
        nlt::bezier5(p0, p1, c0, c1, c2, c3, end), 0xff000000);

      const auto sample_curve =
        [lineLength, begin, end, &debugLinesGraph, x_begin,
         x_end](auto fn, const uint32_t col = 0xff000000) {
          debugLinesGraph.addLine(
            as::vec3(x_begin, as::mix(0.0f, lineLength, fn(begin)), 0.0f),
            as::vec3(x_end, as::mix(0.0f, lineLength, fn(end)), 0.0f), col);
        };

      if (linear) {
        sample_curve([](const float a) { return a; });
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
    t += dt / time * direction;
    if (t >= 1.0f || t <= 0.0f) {
      t = as::clamp(t, 0.0f, 1.0f);
      direction *= -1.0f;
    }

    const auto start = as::vec3(2.0f, -1.5, 0.0f);
    const auto end = as::vec3(18.0f, -1.5, 0.0f);

    debugLinesGraph.addLine(start, end, 0xff000000);
    debugLinesGraph.submit();

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

    const auto position = as::vec_mix(start, end, interpolations[item](t));
    auto moving_circle = dbg::DebugCircle(
      as::mat4_from_mat3_vec3(as::mat3::identity(), position),
      dbg::CurveHandles::HandleRadius, main_view, program_col);
    moving_circle.draw();
    // animation end

    // screen space drawing
    float view_o[16];
    as::mat_to_arr(as::mat4::identity(), view_o);

    const as::mat4 orthographic_projection =
      as::ortho_d3d_lh(0.0f, width, height, 0.0f, 0.0f, 1.0f);

    float proj_o[16];
    as::mat_to_arr(orthographic_projection, proj_o);
    bgfx::setViewTransform(ortho_view, view_o, proj_o);

    const auto start_screen = worldToScreen(
      start, perspective_projection, camera.view(), screen_dimension);
    const auto end_screen = worldToScreen(
      end, perspective_projection, camera.view(), screen_dimension);

    dbg::DebugLines dl_screen(ortho_view, program_col);
    dl_screen.addLine(
      as::vec3(start_screen.x, start_screen.y - 10.0f, 0.0f),
      as::vec3(start_screen.x, start_screen.y + 10.0f, 0.0f), 0xff555555);
    dl_screen.addLine(
      as::vec3(end_screen.x, end_screen.y - 10.0f, 0.0f),
      as::vec3(end_screen.x, end_screen.y + 10.0f, 0.0f), 0xff555555);
    dl_screen.submit();

    ImGui::Text("Framerate: ");
    ImGui::SameLine(100);
    ImGui::Text("%f", framerate);

    // include this in case nothing was submitted to draw
    bgfx::touch(main_view);
    bgfx::touch(ortho_view);

    ImGui::Render();
    bgfx::frame();
  }

  bgfx::destroy(program_col);

  ImGui_ImplSDL2_Shutdown();
  ImGui_Implbgfx_Shutdown();

  ImGui::DestroyContext();
  bgfx::shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
