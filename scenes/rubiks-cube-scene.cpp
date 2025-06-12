#include "rubiks-cube-scene.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>

void rubiks_cube_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height) {
  main_view_ = main_view;
  ortho_view_ = ortho_view;
  screen_dimension_ = {width, height};

  perspective_projection_ = as::perspective_direct3d_lh(
    as::radians(60.0f), float(width) / float(height), 0.01f, 1000.0f);

  asci::Cameras& cameras = camera_system_.cameras_;
  cameras.addCamera(&first_person_rotate_camera_);
  cameras.addCamera(&first_person_translate_camera_);

  camera_ = target_camera_;
}

void rubiks_cube_scene_t::input(const SDL_Event& current_event) {
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void rubiks_cube_scene_t::update(
  debug_draw_t& debug_draw, const float delta_time) {
  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ = asci::smoothCamera(
    camera_, target_camera_, asci::SmoothProps{}, delta_time);

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);
}
