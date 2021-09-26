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

void simple_camera_scene_t::update(debug_draw_t& debug_draw)
{
  const auto freq = double(bx::getHPFrequency());
  const auto now = bx::getHPCounter();
  const auto delta = now - prev_;
  prev_ = now;

  const float delta_time = delta / static_cast<float>(freq);

  ImGui::Begin("Simple Camera");

  static float speed = 10.0f;

  static float pitch = 0.0f;
  ImGui::SliderFloat("pitch", &pitch, -as::k_pi / 2.0f, as::k_pi / 2.0f);
  camera_.pitch = pitch;

  static float yaw = 0.0f;
  ImGui::SliderFloat("yaw", &yaw, -as::k_tau, as::k_tau);
  camera_.yaw = yaw;

  static bool detached_pivot = false;
  ImGui::Checkbox("detached pivot", &detached_pivot);

  std::function<void(const as::vec3& pivot)> detached_move =
    [this](const as::vec3& pivot_delta) mutable {
      asc::move_pivot_detached(camera_, camera_.pivot + pivot_delta);
    };

  std::function<void(const as::vec3& pivot)> attached_move =
    [this](const as::vec3& pivot_delta) mutable {
      camera_.pivot += pivot_delta;
    };

  std::function<void(const as::vec3& pivot)> pivot_set_fn =
    detached_pivot ? detached_move : attached_move;

  ImGui::PushID("pivot");
  ImGui::Button("forward");
  if (ImGui::IsItemActive()) {
    pivot_set_fn(camera_.rotation() * as::vec3::axis_z() * delta_time * speed);
  }
  ImGui::SameLine();
  ImGui::Button("back");
  if (ImGui::IsItemActive()) {
    pivot_set_fn(camera_.rotation() * -as::vec3::axis_z() * delta_time * speed);
  }
  ImGui::SameLine();
  ImGui::Button("up");
  if (ImGui::IsItemActive()) {
    pivot_set_fn(as::vec3::axis_y() * delta_time * speed);
  }
  ImGui::SameLine();
  ImGui::Button("down");
  if (ImGui::IsItemActive()) {
    pivot_set_fn(-as::vec3::axis_y() * delta_time * speed);
  }
  ImGui::SameLine();
  ImGui::Button("left");
  if (ImGui::IsItemActive()) {
    pivot_set_fn(camera_.rotation() * -as::vec3::axis_x() * delta_time * speed);
  }
  ImGui::SameLine();
  ImGui::Button("right");
  if (ImGui::IsItemActive()) {
    pivot_set_fn(camera_.rotation() * as::vec3::axis_x() * delta_time * speed);
  }
  ImGui::PopID();

  ImGui::SameLine();
  ImGui::Text(
    "pivot (%.2f, %.2f, %.2f)", camera_.pivot.x, camera_.pivot.y,
    camera_.pivot.z);

  ImGui::PushID("offset");
  ImGui::Button("forward");
  if (ImGui::IsItemActive()) {
    camera_.offset += as::vec3::axis_z() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("back");
  if (ImGui::IsItemActive()) {
    camera_.offset -= as::vec3::axis_z() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("up");
  if (ImGui::IsItemActive()) {
    camera_.offset += as::mat_transpose(camera_.rotation()) * as::vec3::axis_y()
                    * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("down");
  if (ImGui::IsItemActive()) {
    camera_.offset -= as::mat_transpose(camera_.rotation()) * as::vec3::axis_y()
                    * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("left");
  if (ImGui::IsItemActive()) {
    camera_.offset -= as::vec3::axis_x() * delta_time * speed;
  }
  ImGui::SameLine();
  ImGui::Button("right");
  if (ImGui::IsItemActive()) {
    camera_.offset += as::vec3::axis_x() * delta_time * speed;
  }
  ImGui::PopID();

  ImGui::SameLine();
  ImGui::Text(
    "offset (%.2f, %.2f, %.2f)", camera_.offset.x, camera_.offset.y,
    camera_.offset.z);

  ImGui::Text(
    "translation (%.2f, %.2f, %.2f)", camera_.translation().x,
    camera_.translation().y, camera_.translation().z);

  ImGui::End();

  drawGrid(*debug_draw.debug_lines, camera_.translation());

  const float alpha =
    as::min(as::vec_distance(camera_.pivot, camera_.translation()), 2.0f)
    / 2.0f;
  debug_draw.debug_spheres->addSphere(
    as::mat4_from_vec3(camera_.pivot), as::vec4(as::vec3::one(), alpha));
  drawTransform(
    *debug_draw.debug_lines, as::affine_from_vec3(camera_.pivot), alpha);

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);

  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);
  bgfx::setViewTransform(main_view_, view, proj);

  bgfx::touch(main_view_);
}
