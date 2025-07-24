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

static csg_t create_csg() {

  const auto csg_1 = csg_cube(
    csg_cube_config_t{
      .min = as::vec3f(-5.0f, -10.0f, -2.5f),
      .max = as::vec3f(5.0f, 10.0f, 2.5f),
      .transform = as::affine_from_mat3_vec3(
        as::mat3_rotation_x(as::k_half_pi * 0.5f),
        as::vec3(0.0f, 0.0f, 5.0f))});

  const auto csg_2 = csg_sphere(
    csg_sphere_config_t{
      .position = as::vec3f(0.0f, 3.0f, -0.5f), .radius = 1.5f});

  const auto csg_3 = csg_cube(
    csg_cube_config_t{
      .min = as::vec3f(-5.0f, -5.0f, 0.0f),
      .max = as::vec3f(5.0f, 5.0f, 10.0f),
      .transform =
        as::affine_from_mat3(as::mat3_rotation_z(as::k_half_pi * 0.5f))});

  return csg_union(csg_union(csg_1, csg_2), csg_3);

  const auto csg_4 =
    // csg_cube(as::vec3f::zero(), as::vec3f(2.5f, 0.2f, 0.2f));
    csg_sphere(
      csg_sphere_config_t{
        .position = as::vec3f(-0.5, 0.0, -0.5), .radius = 0.8f});
  const auto temp = csg_subtract(csg_cylinder(csg_cylinder_config_t{}), csg_2);
  // const auto csg_result =
  //   csg_subtract(csg_cylinder(), csg_1); // csg_subtract(temp,
  //   csg_4);
  // const auto polygons = temp.polygons;
  // csg_subtract(csg_subtract(temp, csg_4), csg_1).polygons;
  // csg_subtract(csg_cylinder(csg_cylinder_config_t{}), csg_1)
  //   .polygons; // csg_subtract(temp, csg_4);

  const auto a = csg_cube();
  const auto b = csg_cylinder();
  const auto c = csg_cylinder(
    {.start = {-as::vec3f::axis_x()}, .end = {as::vec3f::axis_x()}});
  const auto d = csg_cylinder(
    {.start = {-as::vec3f::axis_z()}, .end = {as::vec3f::axis_z()}});
  auto inverse = csg_inverse(csg_union(csg_union(b, c), d));

  return inverse;
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

  target_camera_.pivot = as::vec3(0.0f, 15.0f, -20.0f);
  target_camera_.pitch = as::radians(30.0f);
  camera_ = target_camera_;

  render_thing_t::init();
  render_thing =
    render_thing_from_csg(create_csg(), as::vec3f(1.0f, 1.0f, 0.0f));

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
  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ = asci::smoothCamera(
    camera_, target_camera_, asci::SmoothProps{}, delta_time);

  render_thing_debug(render_thing, *debug_draw.debug_lines);

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  drawGrid(*debug_draw.debug_lines, as::vec3::zero());

  const auto offset = as::mat4::identity();

  float model[16];
  as::mat_to_arr(offset, model);
  bgfx::setTransform(model);

  render_thing_draw(render_thing, main_view_);

  bgfx::setUniform(u_light_pos_, (void*)&light_pos_, 1);
  bgfx::setUniform(u_camera_pos_, (void*)&camera_.pivot, 1);
}

void csg_scene_t::teardown() {
  bgfx::destroy(u_light_pos_);
  bgfx::destroy(u_camera_pos_);
  destroy_render_thing(render_thing);
  render_thing_t::deinit();
}
