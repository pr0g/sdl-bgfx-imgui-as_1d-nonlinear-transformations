#include "bgfx-imgui/imgui_impl_bgfx.h"

#include "hierarchy-scene.h"
#include "marching-cube-scene.h"
#include "sdl-imgui/imgui_impl_sdl.h"
#include "transforms-scene.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <imgui.h>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-line.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>
#include <thh-bgfx-debug/debug-sphere.hpp>

#include <optional>
#include <tuple>

namespace asc
{

Handedness handedness()
{
  return Handedness::Left;
}

} // namespace asc

int main(int argc, char** argv)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  const int width = 1024;
  const int height = 768;
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

  const char* scene_names[] = {
    "Non-Linear Transformations", "Marching Cubes", "Hierarchy"};

  std::unique_ptr<scene_t> scene = nullptr;
  auto scene_builder = [](const int scene_id) -> std::unique_ptr<scene_t> {
    switch (scene_id) {
      case 0:
        return std::make_unique<transforms_scene_t>();
      case 1:
        return std::make_unique<marching_cube_scene_t>();
      case 2:
        return std::make_unique<imgui_hierarchy_scene_t>();
      default:
        return nullptr;
    }
  };

  ImGui::CreateContext();
  ImGui_Implbgfx_Init(255);
#if BX_PLATFORM_WINDOWS
  ImGui_ImplSDL2_InitForD3D(window);
#elif BX_PLATFORM_OSX
  ImGui_ImplSDL2_InitForMetal(window);
#elif BX_PLATFORM_LINUX
  ImGui_ImplSDL2_InitForOpenGL(window, nullptr);
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX

  dbg::DebugVertex::init();
  dbg::DebugCircles::init();
  dbg::DebugCubes::init();

  enum class mode_e
  {
    waiting_for_scene,
    running_scene
  };

  bgfx::ViewId main_view = 0;
  bgfx::ViewId ortho_view = 1;

  int scene_index = 0;
  mode_e mode = mode_e::waiting_for_scene;

  bgfx::setViewClear(
    main_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
  bgfx::setViewRect(main_view, 0, 0, width, height);
  bgfx::setViewClear(ortho_view, BGFX_CLEAR_DEPTH);
  bgfx::setViewRect(ortho_view, 0, 0, width, height);

  for (bool quit = false; !quit;) {
    SDL_Event current_event;
    while (SDL_PollEvent(&current_event) != 0) {
      ImGui_ImplSDL2_ProcessEvent(&current_event);
      if (current_event.type == SDL_QUIT) {
        quit = true;
        break;
      }

      if (scene) {
        scene->input(current_event);
      }

      if (current_event.type == SDL_KEYDOWN) {
        const auto* keyboard_event = (SDL_KeyboardEvent*)&current_event;
        if (keyboard_event->keysym.scancode == SDL_SCANCODE_ESCAPE) {
          scene->teardown();
          scene.reset();
          mode = mode_e::waiting_for_scene;
        }
      }
    }

    ImGui_Implbgfx_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    switch (mode) {
      case mode_e::waiting_for_scene:
        ImGui::Combo(
          "Scene Select", &scene_index, scene_names, std::size(scene_names));
        if (ImGui::Button("Launch Scene")) {
          scene = scene_builder(scene_index);
          scene->setup(main_view, ortho_view, width, height);
          mode = mode_e::running_scene;
        }
        break;
      case mode_e::running_scene: {
        dbg::DebugLines debug_lines(main_view, scene->simple_handle());
        dbg::DebugLines debug_lines_screen(ortho_view, scene->simple_handle());
        dbg::DebugCircles debug_circles(main_view, scene->instance_handle());
        dbg::DebugSpheres debug_spheres(debug_circles);
        const size_t quad_dimension = 100;
        dbg::DebugQuads debug_quads(main_view, scene->instance_handle());
        debug_quads.reserveQuads(quad_dimension * quad_dimension);
        dbg::DebugCubes debug_cubes(main_view, scene->instance_handle());

        debug_draw_t debug_draw{&debug_circles, &debug_spheres,
                                &debug_lines,   &debug_lines_screen,
                                &debug_cubes,   &debug_quads};

        scene->update(debug_draw);

        debug_lines.submit();
        debug_lines_screen.submit();
        debug_quads.submit();
        debug_circles.submit();
        debug_cubes.submit();
      } break;
    }

    bgfx::touch(main_view);
    bgfx::touch(ortho_view);

    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

    bgfx::frame();
  }

  if (scene) {
    scene->teardown();
  }

  ImGui_ImplSDL2_Shutdown();
  ImGui_Implbgfx_Shutdown();

  ImGui::DestroyContext();
  bgfx::shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
