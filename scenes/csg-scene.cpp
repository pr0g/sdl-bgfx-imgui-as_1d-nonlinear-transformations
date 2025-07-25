#include "csg-scene.h"

#include "bgfx-helpers.h"
#include "csg/csg.h"
#include "debug.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <imgui.h>

#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>

#include <ranges>

static csg_t create_csg_union() {
  const auto a =
    csg_cube({.min = {-1.25f, -1.25f, -1.25f}, .max = {0.75f, 0.75f, 0.75f}});
  const auto b = csg_sphere({.center = {0.25f, 0.25f, 0.25f}, .radius = 1.3f});
  return csg_union(a, b);
}

static csg_t create_csg_subtract() {
  const auto a =
    csg_cube({.min = {-1.25f, -1.25f, -1.25f}, .max = {0.75f, 0.75f, 0.75f}});
  const auto b = csg_sphere({.center = {0.25f, 0.25f, 0.25f}, .radius = 1.3f});
  return csg_subtract(a, b);
}

static csg_t create_csg_intersect() {
  const auto a =
    csg_cube({.min = {-1.25, -1.25, -1.25}, .max = {0.75f, 0.75f, 0.75f}});
  const auto b = csg_sphere({.center = {0.25f, 0.25f, 0.25f}, .radius = 1.3f});
  return csg_intersect(a, b);
}

static csg_t create_csg_classic() {
  const auto a = csg_cube();
  const auto b = csg_sphere({.radius = 1.35f, .stacks = 12});
  const auto c = csg_cylinder(
    {.radius = 0.7f,
     .start = as::vec3f(-1.0f, 0.0f, 0.0f),
     .end = as::vec3f(1.0f, 0.0f, 0.0f)});
  const auto d = csg_cylinder(
    {.radius = 0.7f,
     .start = as::vec3f(0.0f, -1.0f, 0.0f),
     .end = as::vec3f(0.0f, 1.0f, 0.0f)});
  const auto e = csg_cylinder(
    {.radius = 0.7f,
     .start = as::vec3f(0.0f, 0.0f, -1.0f),
     .end = as::vec3f(0.0f, 0.0f, 1.0f)});
  return csg_subtract(csg_intersect(a, b), csg_union(csg_union(c, d), e));
}

void csg_scene_t::setup(
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

  target_camera_.pivot = as::vec3(0.0f, 8.0f, -10.0f);
  target_camera_.pitch = as::radians(30.0f);
  camera_ = target_camera_;

  render_thing_t::init();
  render_things_.push_back(render_thing_from_csg(
    create_csg_union(),
    as::mat4_from_mat3_vec3(
      as::mat3_rotation_y(as::k_half_pi), as::vec3f::axis_x(-5.0f)),
    as::vec3f(1.0f, 1.0f, 0.0f)));
  render_things_.push_back(render_thing_from_csg(
    create_csg_subtract(),
    as::mat4_from_mat3(as::mat3_rotation_y(as::k_half_pi)),
    as::vec3f(1.0f, 1.0f, 0.0f)));
  render_things_.push_back(render_thing_from_csg(
    create_csg_intersect(),
    as::mat4_from_mat3_vec3(
      as::mat3_rotation_y(as::k_half_pi), as::vec3f::axis_x(5.0f)),
    as::vec3f(1.0f, 1.0f, 0.0f)));
  render_things_.push_back(render_thing_from_csg(
    create_csg_classic(),
    as::mat4_from_mat3_vec3(
      as::mat3_rotation_y(as::k_half_pi), as::vec3f::axis_y(5.0f)),
    as::vec3f(1.0f, 1.0f, 0.0f)));

  u_light_pos_ = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4, 1);
  u_camera_pos_ =
    bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4, 1);
}

void csg_scene_t::input(const SDL_Event& current_event) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void csg_scene_t::update(debug_draw_t& debug_draw, const float delta_time) {
  ImGui::Begin("CSG");

  ImGui::Checkbox("Wireframe", &wireframe_);
  ImGui::Checkbox("Normals", &normals_);

  ImGui::End();

  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ = asci::smoothCamera(
    camera_, target_camera_, asci::SmoothProps{}, delta_time);

  for (const auto& render_thing : render_things_) {
    render_thing_debug(
      render_thing, *debug_draw.debug_lines,
      {.normals = normals_, .wireframe = wireframe_});
  }

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  drawGrid(*debug_draw.debug_lines, as::vec3::zero());

  const auto offset = as::mat4::identity();

  bgfx::setUniform(u_light_pos_, (void*)&light_pos_, 1);
  bgfx::setUniform(u_camera_pos_, (void*)&camera_.pivot, 1);

  for (const auto& render_thing : render_things_) {
    render_thing_draw(render_thing, main_view_);
  }
}

void csg_scene_t::teardown() {
  bgfx::destroy(u_light_pos_);
  bgfx::destroy(u_camera_pos_);
  for (const auto& render_thing : render_things_) {
    destroy_render_thing(render_thing);
  }
  render_thing_t::deinit();
}
