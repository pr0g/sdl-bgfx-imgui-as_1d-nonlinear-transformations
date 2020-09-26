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

    static bool linear = false;
    ImGui::Checkbox("Linear", &linear);
    static bool smooth_step = false;
    ImGui::Checkbox("Smooth Step", &smooth_step);
    static bool smoother_step = false;
    ImGui::Checkbox("Smoother Step", &smoother_step);
    static float smooth_stop_start_mix_t = 0.0f;
    ImGui::SliderFloat("t", &smooth_stop_start_mix_t, 0.0f, 1.0f);
    static bool smooth_stop_start_mix2 = false;
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

    static bool smooth_start2 = false;
    static bool smooth_start3 = false;
    static bool smooth_start4 = false;
    static bool smooth_start5 = false;
    ImGui::Checkbox("Smooth Start 2", &smooth_start2);
    ImGui::Checkbox("Smooth Start 3", &smooth_start3);
    ImGui::Checkbox("Smooth Start 4", &smooth_start4);
    ImGui::Checkbox("Smooth Start 5", &smooth_start5);
    static bool smooth_stop2 = false;
    static bool smooth_stop3 = false;
    static bool smooth_stop4 = false;
    static bool smooth_stop5 = false;
    ImGui::Checkbox("Smooth Stop 2", &smooth_stop2);
    ImGui::Checkbox("Smooth Stop 3", &smooth_stop3);
    ImGui::Checkbox("Smooth Stop 4", &smooth_stop4);
    ImGui::Checkbox("Smooth Stop 5", &smooth_stop5);

    auto debugLinesGraph = dbg::DebugLines(main_view, program_col);
    const auto lineGranularity = 20;
    const auto lineLength = 20.0f;
    for (auto i = 0; i < lineGranularity; ++i) {
      float begin = i / float(lineGranularity);
      float end = (i + 1) / float(lineGranularity);

      float x_begin = begin * lineLength;
      float x_end = end * lineLength;

      if (linear) {
        auto linear_begin = as::mix(0.0f, lineLength, begin);
        auto linear_end = as::mix(0.0f, lineLength, end);

        debugLinesGraph.addLine(
          as::vec3(x_begin, linear_begin, 0.0f),
          as::vec3(x_end, linear_end, 0.0f), 0xff000000);
      }

      if (smooth_step) {
        auto smooth_begin = as::mix(0.0f, lineLength, as::smooth_step(begin));
        auto smooth_end = as::mix(0.0f, lineLength, as::smooth_step(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_begin, 0.0f),
          as::vec3(x_end, smooth_end, 0.0f), 0xff000000);
      }

      if (smoother_step) {
        auto smoother_begin =
          as::mix(0.0f, lineLength, as::smoother_step(begin));
        auto smoother_end = as::mix(0.0f, lineLength, as::smoother_step(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smoother_begin, 0.0f),
          as::vec3(x_end, smoother_end, 0.0f), 0xff000000);
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
          as::vec3(x_end, smooth_start_stop_begin_end, 0.0f), 0xff000000);
      }

      if (smooth_start2) {
        auto smooth_begin = as::mix(0.0f, lineLength, nlt::smoothStart2(begin));
        auto smooth_end = as::mix(0.0f, lineLength, nlt::smoothStart2(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_begin, 0.0f),
          as::vec3(x_end, smooth_end, 0.0f), 0xff000000);
      }

      if (smooth_start3) {
        auto smooth_begin = as::mix(0.0f, lineLength, nlt::smoothStart3(begin));
        auto smooth_end = as::mix(0.0f, lineLength, nlt::smoothStart3(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_begin, 0.0f),
          as::vec3(x_end, smooth_end, 0.0f), 0xff000000);
      }

      if (smooth_start4) {
        auto smooth_begin = as::mix(0.0f, lineLength, nlt::smoothStart4(begin));
        auto smooth_end = as::mix(0.0f, lineLength, nlt::smoothStart4(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_begin, 0.0f),
          as::vec3(x_end, smooth_end, 0.0f), 0xff000000);
      }

      if (smooth_start5) {
        auto smooth_begin = as::mix(0.0f, lineLength, nlt::smoothStart5(begin));
        auto smooth_end = as::mix(0.0f, lineLength, nlt::smoothStart5(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_begin, 0.0f),
          as::vec3(x_end, smooth_end, 0.0f), 0xff000000);
      }

      if (smooth_stop2) {
        auto smooth_begin = as::mix(0.0f, lineLength, nlt::smoothStop2(begin));
        auto smooth_end = as::mix(0.0f, lineLength, nlt::smoothStop2(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_begin, 0.0f),
          as::vec3(x_end, smooth_end, 0.0f), 0xff000000);
      }

      if (smooth_stop3) {
        auto smooth_begin = as::mix(0.0f, lineLength, nlt::smoothStop3(begin));
        auto smooth_end = as::mix(0.0f, lineLength, nlt::smoothStop3(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_begin, 0.0f),
          as::vec3(x_end, smooth_end, 0.0f), 0xff000000);
      }

      if (smooth_stop4) {
        auto smooth_begin = as::mix(0.0f, lineLength, nlt::smoothStop4(begin));
        auto smooth_end = as::mix(0.0f, lineLength, nlt::smoothStop4(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_begin, 0.0f),
          as::vec3(x_end, smooth_end, 0.0f), 0xff000000);
      }

      if (smooth_stop5) {
        auto smooth_begin = as::mix(0.0f, lineLength, nlt::smoothStop5(begin));
        auto smooth_end = as::mix(0.0f, lineLength, nlt::smoothStop5(end));

        debugLinesGraph.addLine(
          as::vec3(x_begin, smooth_begin, 0.0f),
          as::vec3(x_end, smooth_end, 0.0f), 0xff000000);
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
