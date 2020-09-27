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
#include "debug-lines.h"
#include "file-ops.h"
#include "fps.h"
#include "imgui.h"
#include "sdl-imgui/imgui_impl_sdl.h"

#include <algorithm>
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

int main(int argc, char** argv)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  const int width = 800;
  const int height = 600;
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

  const bgfx::ViewId main_view = 0;

  // cornflower clear color
  bgfx::setViewClear(
    main_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
  bgfx::setViewRect(main_view, 0, 0, width, height);
  // dummy draw call to make sure main_view is cleared if no other draw calls
  // are submitted
  bgfx::touch(main_view);

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
  camera.look_at = as::point3(14.5f, 9.2f, -21.0f);

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

  // debug settings
  bool linear = false;
  bool smooth_step = false;
  bool smoother_step = false;
  bool smooth_stop_start_mix2 = false;
  bool smooth_start2 = false;
  bool smooth_start3 = false;
  bool smooth_start4 = false;
  bool smooth_start5 = false;
  bool smooth_stop2 = false;
  bool smooth_stop3 = false;
  bool smooth_stop4 = false;
  bool smooth_stop5 = false;
  bool bezier_smooth_step = false;
  bool normalized_bezier2 = false;
  bool normalized_bezier3 = false;
  bool normalized_bezier4 = false;
  bool normalized_bezier5 = false;

  float smooth_stop_start_mix_t = 0.0f;
  float normalized_bezier_b = 0.0f;
  float normalized_bezier_c = 0.0f;
  float normalized_bezier_d = 0.0f;
  float normalized_bezier_e = 0.0f;

  auto prev = bx::getHPCounter();

  fps::Fps fps;
  for (bool quit = false; !quit;) {
    SDL_Event current_event;
    while (SDL_PollEvent(&current_event) != 0) {
      updateCameraControlKeyboardSdl(
        current_event, camera_control, camera_props);
      ImGui_ImplSDL2_ProcessEvent(&current_event);
      if (current_event.type == SDL_QUIT) {
        quit = true;
        break;
      }
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

    const as::mat4 perspective_projection = as::perspective_d3d_lh(
      as::radians(60.0f), float(width) / float(height), 0.01f, 100.0f);

    float proj[16];
    as::mat_to_arr(perspective_projection, proj);
    bgfx::setViewTransform(main_view, view, proj);

    ImGui::Checkbox("Linear", &linear);
    ImGui::Checkbox("Smooth Step", &smooth_step);
    ImGui::Checkbox("Smoother Step", &smoother_step);
    ImGui::SliderFloat("t", &smooth_stop_start_mix_t, 0.0f, 1.0f);
    ImGui::Checkbox("Smooth Stop Start Mix 2", &smooth_stop_start_mix2);
    const float smooth_stop_start_mix = as::mix(
      nlt::smoothStart2(smooth_stop_start_mix_t),
      nlt::smoothStop2(smooth_stop_start_mix_t), smooth_stop_start_mix_t);
    ImGui::Text("Smooth Stop Start Mix 2: ");
    ImGui::SameLine(180);
    ImGui::Text("%f", smooth_stop_start_mix);
    ImGui::Text("Smooth Step: ");
    ImGui::SameLine(100);
    ImGui::Text("%f", as::smooth_step(smooth_stop_start_mix_t));
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

    auto debugLinesGraph = dbg::DebugLines(main_view, program_col);
    const auto lineGranularity = 20;
    const auto lineLength = 20.0f;
    for (auto i = 0; i < lineGranularity; ++i) {
      float begin = i / float(lineGranularity);
      float end = (i + 1) / float(lineGranularity);

      float x_begin = begin * lineLength;
      float x_end = end * lineLength;

      auto sample_curve = [lineLength, begin, end, &debugLinesGraph, x_begin,
                           x_end](float (*fn)(float)) {
        auto sample_begin = as::mix(0.0f, lineLength, fn(begin));
        auto sample_end = as::mix(0.0f, lineLength, fn(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, sample_begin, 0.0f),
          as::vec3(x_end, sample_end, 0.0f), 0xff000000);
      };

      if (linear) {
        sample_curve([](const float a) { return a; });
      }

      if (smooth_step) {
        sample_curve(as::smooth_step);
      }

      if (smoother_step) {
        sample_curve(as::smoother_step);
      }

      if (smooth_stop_start_mix2) {
        auto smooth_start_stop_begin = as::mix(
          0.0f, lineLength,
          as::mix(
            nlt::smoothStart2(begin), nlt::smoothStop2(begin),
            smooth_stop_start_mix_t));
        auto smooth_start_stop_begin_end = as::mix(
          0.0f, lineLength,
          as::mix(
            nlt::smoothStart2(end), nlt::smoothStop2(end),
            smooth_stop_start_mix_t));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_start_stop_begin, 0.0f),
          as::vec3(x_end, smooth_start_stop_begin_end, 0.0f), 0xff00ff00);
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
        auto sample_begin = as::mix(
          0.0f, lineLength, nlt::normalizedBezier2(normalized_bezier_b, begin));
        auto sample_end = as::mix(
          0.0f, lineLength, nlt::normalizedBezier2(normalized_bezier_b, end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, sample_begin, 0.0f),
          as::vec3(x_end, sample_end, 0.0f), 0xff0000ff);
      }

      if (normalized_bezier3) {
        auto sample_begin = as::mix(
          0.0f, lineLength,
          nlt::normalizedBezier3(
            normalized_bezier_b, normalized_bezier_c, begin));
        auto sample_end = as::mix(
          0.0f, lineLength,
          nlt::normalizedBezier3(
            normalized_bezier_b, normalized_bezier_c, end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, sample_begin, 0.0f),
          as::vec3(x_end, sample_end, 0.0f), 0xff0000ff);
      }

      if (normalized_bezier4) {
        auto sample_begin = as::mix(
          0.0f, lineLength,
          nlt::normalizedBezier4(
            normalized_bezier_b, normalized_bezier_c, normalized_bezier_d,
            begin));
        auto sample_end = as::mix(
          0.0f, lineLength,
          nlt::normalizedBezier4(
            normalized_bezier_b, normalized_bezier_c, normalized_bezier_d,
            end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, sample_begin, 0.0f),
          as::vec3(x_end, sample_end, 0.0f), 0xff0000ff);
      }

      if (normalized_bezier5) {
        auto sample_begin = as::mix(
          0.0f, lineLength,
          nlt::normalizedBezier5(
            normalized_bezier_b, normalized_bezier_c, normalized_bezier_d,
            normalized_bezier_e, begin));
        auto sample_end = as::mix(
          0.0f, lineLength,
          nlt::normalizedBezier5(
            normalized_bezier_b, normalized_bezier_c, normalized_bezier_d,
            normalized_bezier_e, end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, sample_begin, 0.0f),
          as::vec3(x_end, sample_end, 0.0f), 0xff0000ff);
      }
    }

    debugLinesGraph.submit();

    ImGui::Text("Framerate: ");
    ImGui::SameLine(100);
    ImGui::Text("%f", framerate);

    // include this in case nothing was submitted to draw
    bgfx::touch(main_view);

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
