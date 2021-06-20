#include "bgfx-imgui/imgui_impl_bgfx.h"

#include "scenes/hierarchy-scene.h"
#include "scenes/marching-cube-scene.h"
#include "scenes/transforms-scene.h"
#include "sdl-imgui/imgui_impl_sdl.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-line.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>
#include <thh-bgfx-debug/debug-sphere.hpp>

#include <fstream>
#include <optional>
#include <tuple>

using json = nlohmann::json;

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

  {
    // note: must be destructed before bgfx shutdown
    dbg::DebugLines debug_lines;
    dbg::DebugLines debug_lines_screen;
    dbg::DebugCircles debug_circles;
    dbg::DebugSpheres debug_spheres(debug_circles);
    dbg::DebugQuads debug_quads;
    dbg::DebugCubes debug_cubes;

    // overall mode for app
    enum class mode_e
    {
      waiting_for_scene,
      running_scene
    };

    dbg::EmbeddedShaderProgram simple_program;
    simple_program.init(dbg::SimpleEmbeddedShaderArgs);
    dbg::EmbeddedShaderProgram instance_program;
    instance_program.init(dbg::InstanceEmbeddedShaderArgs);

    bgfx::ViewId main_view = 0;
    bgfx::ViewId ortho_view = 1;

    std::ifstream settings_input("settings.json");
    json settings;
    settings_input >> settings;

    int scene_index = [&settings]() -> int {
      if (settings.contains("scene-index")) {
        return settings.at("scene-index");
      }
      return -1;
    }();

    mode_e mode = mode_e::waiting_for_scene;
    auto begin_scene = [&](const int scene_index) {
      scene = scene_builder(scene_index);
      scene->setup(main_view, ortho_view, width, height);

      debug_lines.setRenderContext(main_view, simple_program.handle());
      debug_lines_screen.setRenderContext(ortho_view, simple_program.handle());
      debug_circles.setRenderContext(main_view, instance_program.handle());
      debug_quads.setRenderContext(main_view, instance_program.handle());
      debug_cubes.setRenderContext(main_view, instance_program.handle());

      mode = mode_e::running_scene;
    };

    if (scene_index != -1) {
      begin_scene(scene_index);
    } else {
      scene_index = 0;
    }

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
          if (current_event.type == SDL_KEYDOWN) {
            const auto* keyboard_event = (SDL_KeyboardEvent*)&current_event;
            if (keyboard_event->keysym.scancode == SDL_SCANCODE_ESCAPE) {
              scene->teardown();
              scene.reset();
              mode = mode_e::waiting_for_scene;
            }
          }
        }
      }

      ImGui_Implbgfx_NewFrame();
      ImGui_ImplSDL2_NewFrame(window);
      ImGui::NewFrame();

      switch (mode) {
        case mode_e::waiting_for_scene:
          ImGui::Begin("Scene Select");
          ImGui::Combo(
            "Scene Select", &scene_index, scene_names, std::size(scene_names));
          if (ImGui::Button("Launch Scene")) {
            begin_scene(scene_index);
          }
          ImGui::End();
          break;
        case mode_e::running_scene: {
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

    simple_program.deinit();
    instance_program.deinit();
  }

  ImGui_ImplSDL2_Shutdown();
  ImGui_Implbgfx_Shutdown();

  ImGui::DestroyContext();
  bgfx::shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
