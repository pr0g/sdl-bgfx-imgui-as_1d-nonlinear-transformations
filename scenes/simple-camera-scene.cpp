#include "simple-camera-scene.h"

#include "debug.h"

#include <SDL.h>
#include <as/as-view.hpp>
#include <bx/timer.h>
#include <imgui.h>
#include <thh-bgfx-debug/debug-sphere.hpp>

void simple_camera_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  main_view_ = main_view;

  const float fov = as::radians(60.0f);
  perspective_projection_ =
    as::perspective_d3d_lh(fov, float(width) / float(height), 0.01f, 1000.0f);
}

void simple_camera_scene_t::input(const SDL_Event& current_event)
{
}

void simple_camera_scene_t::update(debug_draw_t& debug_draw)
{
  const auto freq = double(bx::getHPFrequency());
  const auto now = bx::getHPCounter();
  const auto delta = now - prev_;
  prev_ = now;

  const float delta_time = delta / static_cast<float>(freq);

  ImGui::Begin("Simple Camera");
  static float pitch = 0.0f;
  ImGui::SliderFloat("pitch", &pitch, -7.0f, 7.0f);
  camera_.pitch = pitch;

  static float yaw = 0.0f;
  ImGui::SliderFloat("yaw", &yaw, -7.0f, 7.0f);
  camera_.yaw = yaw;

  const float speed = 4.0f;

  ImGui::Button("pivot-forward");
  if (ImGui::IsItemActive()) {
    camera_.pivot +=
      camera_.rotation() * as::vec3::axis_z() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("pivot-back");
  if (ImGui::IsItemActive()) {
    camera_.pivot -=
      camera_.rotation() * as::vec3::axis_z() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("pivot-up");
  if (ImGui::IsItemActive()) {
    camera_.pivot += as::vec3::axis_y() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("pivot-down");
  if (ImGui::IsItemActive()) {
    camera_.pivot -= as::vec3::axis_y() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("pivot-left");
  if (ImGui::IsItemActive()) {
    camera_.pivot -=
      camera_.rotation() * as::vec3::axis_x() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("pivot-right");
  if (ImGui::IsItemActive()) {
    camera_.pivot +=
      camera_.rotation() * as::vec3::axis_x() * delta_time * speed;
  }

  ImGui::Button("offset-forward");
  if (ImGui::IsItemActive()) {
    camera_.offset += as::vec3::axis_z() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("offset-back");
  if (ImGui::IsItemActive()) {
    camera_.offset -= as::vec3::axis_z() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("offset-up");
  if (ImGui::IsItemActive()) {
    camera_.offset += as::mat_transpose(camera_.rotation()) * as::vec3::axis_y()
                    * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("offset-down");
  if (ImGui::IsItemActive()) {
    camera_.offset -= as::mat_transpose(camera_.rotation()) * as::vec3::axis_y()
                    * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("offset-left");
  if (ImGui::IsItemActive()) {
    camera_.offset -= as::vec3::axis_x() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("offset-right");
  if (ImGui::IsItemActive()) {
    camera_.offset += as::vec3::axis_x() * delta_time * speed;
  }

  ImGui::End();

  drawGrid(*debug_draw.debug_lines, camera_.translation());

  const float alpha = as::min(as::vec_distance(camera_.pivot, camera_.translation()), 2.0f) / 2.0f;
  debug_draw.debug_spheres->addSphere(as::mat4_from_vec3(camera_.pivot), as::vec4(as::vec3::one(), alpha));

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);

  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);
  bgfx::setViewTransform(main_view_, view, proj);

  bgfx::touch(main_view_);
}

void simple_camera_scene_t::teardown()
{
}
