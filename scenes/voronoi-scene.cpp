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

void voronoi_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  main_view_ = main_view;

  const float fov = as::radians(60.0f);
  perspective_projection_ =
    as::perspective_d3d_lh(fov, float(width) / float(height), 0.01f, 1000.0f);

  camera_.pivot = as::vec3(0.0f, 5.0f, -15.0f);
  target_camera_ = camera_;

  cameras_.addCamera(&first_person_rotate_camera_);
  cameras_.addCamera(&first_person_translate_camera_);

  camera_system_.cameras_ = cameras_;
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

  drawGrid(*debug_draw.debug_lines, camera_.translation());

  ImGui::Begin("Voronoi");

  ImGui::SliderFloat("directrix", &directrix_, -10.0f, 20.0f);
  ImGui::SliderFloat("focus", &focus_.y, -10.0f, 20.0f);

  static bool standard = false;
  ImGui::Checkbox("standard", &standard);

  ImGui::End();

  auto y_fn = [=](float x, as::vec2 focus) {
    // equation of parabola from focus and directrix
    // y = 1 / 2(b-k) * (x-a)^2 + 0.5(b+k)
    return ((1.0f / (2.0f * (focus.y - directrix_)))
            * ((x - focus.x) * (x - focus.x)))
         + (0.5f * (focus.y + directrix_));
  };

  // vertex = directrix + (directrix - focus.y)
  auto vertex_delta = (focus_.y - directrix_) * 0.5f;

  auto vertex = as::vec3(focus_.x, directrix_ + vertex_delta, 0.0f);
  // vertex
  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), vertex),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  // standard form
  // (1 / 4p)x^2 - (2h/4p)x + (h^2)/4p + k

  // p = vertex_delta
  auto p = vertex_delta;
  auto h = vertex.x;
  auto k = vertex.y;

  auto y_fn_std = [=](float x) {
    return ((1.0f / (4.0f * p)) * (x * x)) - (((2.0f * h) / (4.0f * p)) * x)
         + (((h * h) / (4.0f * p)) + k);
  };

  const float delta_step = 20.0f / 100.0f;
  float x = -10.0f;
  for (int i = 0; i < 100; ++i) {
    float y = y_fn(x, focus_);
    float ys = y_fn_std(x);
    float x_next = x + delta_step;
    float y_next = y_fn(x + delta_step, focus_);
    float ys_next = y_fn_std(x + delta_step);
    if (!standard) {
      debug_draw.debug_lines->addLine(
        as::vec3(x, y, 0.0f), as::vec3(x_next, y_next, 0.0f), 0xff000000);
    } else {
      debug_draw.debug_lines->addLine(
        as::vec3(x, ys, 0.0f), as::vec3(x_next, ys_next, 0.0f), 0xff0000ff);
    }
    x += delta_step;
  }

  // repeated
  x = -10.0f;
  for (int i = 0; i < 100; ++i) {
    float y = y_fn(x, focus2_);
    float x_next = x + delta_step;
    float y_next = y_fn(x + delta_step, focus2_);
    debug_draw.debug_lines->addLine(
      as::vec3(x, y, 0.0f), as::vec3(x_next, y_next, 0.0f), 0xff000000);
    x += delta_step;
  }

  // focus
  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), as::vec3(focus_)),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), as::vec3(focus2_)),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  // directrix
  debug_draw.debug_lines->addLine(
    as::vec3(-50.0f, directrix_, 0.0f), as::vec3(50.0f, directrix_, 0.0f),
    0xff000000);

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);

  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);
  bgfx::setViewTransform(main_view_, view, proj);

  bgfx::touch(main_view_);
}
