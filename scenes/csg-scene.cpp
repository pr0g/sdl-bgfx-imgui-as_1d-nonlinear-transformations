#include "csg-scene.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <easy_iterator.h>
#include <imgui.h>

#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>

void csg_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height) {
  main_view_ = main_view;
  ortho_view_ = ortho_view;
  screen_dimension_ = {width, height};

  perspective_projection_ = as::perspective_direct3d_lh(
    as::radians(60.0f), float(width) / float(height), 0.01f, 1000.0f);

  asci::Cameras& cameras = camera_system_.cameras_;
  cameras.addCamera(&orbit_rotate_camera_);
  cameras.addCamera(&orbit_dolly_camera_);

  target_camera_.offset = as::vec3(0.0f, 0.0f, -10.0f);
  target_camera_.pitch = as::radians(30.0f);
  target_camera_.yaw = as::radians(45.0f);
  camera_ = target_camera_;
}

void csg_scene_t::input(const SDL_Event& current_event) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void csg_scene_t::update(
  debug_draw_t& debug_draw, const float delta_time) {
}
