#include "voronoi-scene.h"

#include "debug.h"
#include "sdl-camera-input.h"

#include <SDL.h>
#include <as/as-math-ops.hpp>
#include <as/as-view.hpp>
#include <bx/timer.h>
#include <imgui.h>
#include <thh-bgfx-debug/debug-line.hpp>
#include <thh-bgfx-debug/debug-sphere.hpp>

#include <functional>
#include <iostream>

void voronoi_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  main_view_ = main_view;

  const float fov = as::radians(60.0f);
  perspective_projection_ =
    as::perspective_d3d_lh(fov, float(width) / float(height), 0.01f, 1000.0f);

  camera_.pivot = as::vec3(0.0f, 0.0f, -30.0f);
  target_camera_ = camera_;

  cameras_.addCamera(&first_person_rotate_camera_);
  cameras_.addCamera(&first_person_translate_camera_);

  camera_system_.cameras_ = cameras_;

  voronoi_.bounds_.min_ = as::vec2(-10.0f, 10.0f);
  voronoi_.bounds_.max_ = as::vec2(10.0f, -10.0f);

  voronoi_.sites_.push_back(vor::site_t{as::vec2(-5.0f, 5.0f)});
  voronoi_.sites_.push_back(vor::site_t{as::vec2(5.0f, 5.0f)});
  voronoi_.sites_.push_back(vor::site_t{as::vec2(0.0f, 0.0f)});
  voronoi_.sites_.push_back(vor::site_t{as::vec2(-5.0f, -5.0f)});
  voronoi_.sites_.push_back(vor::site_t{as::vec2(5.0f, -5.0f)});

  voronoi_.calculate();
}

void voronoi_scene_t::input(const SDL_Event& current_event)
{
  camera_system_.handleEvents(sdlToInput(&current_event));
}

void voronoi_scene_t::update(debug_draw_t& debug_draw)
{
  const auto freq = double(bx::getHPFrequency());
  const auto now = bx::getHPCounter();
  const auto delta = now - prev_;
  prev_ = now;

  const float delta_time = delta / static_cast<float>(freq);

  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ =
    asci::smoothCamera(camera_, target_camera_, smooth_props_, delta_time);

  voronoi_.display(debug_draw.debug_lines, debug_draw.debug_circles);

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  bgfx::touch(main_view_);
}
