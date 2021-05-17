#include "bgfx-imgui/imgui_impl_bgfx.h"

#include "marching-cube-scene.h"
#include "sdl-imgui/imgui_impl_sdl.h"
#include "transforms-scene.h"
#include "marching-cube-scene.h"
#include "hierarchy-scene.h"

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

  // std::unique_ptr<scene_t> scene = std::make_unique<transforms_scene_t>();
  // std::unique_ptr<scene_t> scene = std::make_unique<marching_cube_scene_t>();
  std::unique_ptr<scene_t> scene = std::make_unique<imgui_hierarchy_scene_t>();
  scene->setup(width, height);

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

  for (; !scene->quit();) {
    SDL_Event current_event;
    while (SDL_PollEvent(&current_event) != 0) {
      ImGui_ImplSDL2_ProcessEvent(&current_event);
      scene->input(current_event);
    }

    ImGui_Implbgfx_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    dbg::DebugLines debug_lines(scene->main_view(), scene->simple_handle());
    dbg::DebugLines debug_lines_screen(
      scene->ortho_view(), scene->simple_handle());
    dbg::DebugCircles debug_circles(
      scene->main_view(), scene->instance_handle());
    dbg::DebugSpheres debug_spheres(debug_circles);
    const size_t quad_dimension = 100;
    dbg::DebugQuads debug_quads(scene->main_view(), scene->instance_handle());
    debug_quads.reserveQuads(quad_dimension * quad_dimension);
    dbg::DebugCubes debug_cubes(scene->main_view(), scene->instance_handle());

    debug_draw_t debug_draw{&debug_circles,      &debug_spheres, &debug_lines,
                            &debug_lines_screen, &debug_cubes,   &debug_quads};

    scene->update(debug_draw);

    debug_lines.submit();
    debug_lines_screen.submit();
    debug_quads.submit();
    debug_circles.submit();
    debug_cubes.submit();

    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

    bgfx::frame();
  }

  scene->teardown();

  ImGui_ImplSDL2_Shutdown();
  ImGui_Implbgfx_Shutdown();

  ImGui::DestroyContext();
  bgfx::shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
