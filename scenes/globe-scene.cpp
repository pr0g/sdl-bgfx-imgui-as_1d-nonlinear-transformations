#include "globe-scene.h"

#include "bgfx-helpers.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>

void globe_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  pos_col_vert_layout_.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();

  main_view_ = main_view;
  ortho_view_ = ortho_view;

  perspective_projection_ = as::perspective_d3d_lh(
    as::radians(60.0f), float(width) / float(height), 0.01f, 100.0f);

  camera_.pivot = as::vec3::axis_z(-10.0f);
  target_camera_ = camera_;

  float current_vertical_angle_rad = as::k_pi / 2.0f;

  const int loop_count = 20;

  // float outer_rot = 0.0f;
  float inner_rot = 0.0f;
  const float horizontal_increment = as::k_tau / 20.0f;
  const float vertical_increment = as::k_pi / 20.0f;
  for (size_t outer = 0; outer < loop_count; ++outer) {
    const float vertical_position_first = std::sin(current_vertical_angle_rad);
    const float vertical_position_second =
      std::sin(current_vertical_angle_rad + vertical_increment);
    const float horizontal_scale_first = std::cos(current_vertical_angle_rad);
    const float horizontal_scale_second =
      std::cos(current_vertical_angle_rad + vertical_increment);
    for (size_t inner = 0; inner < 20; ++inner) {
      sphere_vertices.push_back(
        {as::vec3(
           std::cos(inner_rot) * horizontal_scale_first,
           vertical_position_first,
           std::sin(inner_rot) * horizontal_scale_first),
         0xffffffff});
      sphere_vertices.push_back(
        {as::vec3(
           std::cos(inner_rot) * horizontal_scale_second,
           vertical_position_second,
           std::sin(inner_rot) * horizontal_scale_second),
         0xffffffff});
      inner_rot += horizontal_increment;
    }
    current_vertical_angle_rad += vertical_increment;
  }

  for (int loop = 0; loop < loop_count; ++loop) {
    for (int i = 0; i < 40; i += 2) {
      sphere_indices.push_back(i + (loop * 40));
      sphere_indices.push_back((i + 1) % 40 + (loop * 40));
      sphere_indices.push_back((i + 2) % 40 + (loop * 40));
      sphere_indices.push_back(i + 1 + (loop * 40));
      sphere_indices.push_back((i + 3) % 40 + (loop * 40));
      sphere_indices.push_back((i + 2) % 40 + (loop * 40));
    }
  }

  sphere_col_vbh_ = bgfx::createVertexBuffer(
    bgfx::makeRef(
      sphere_vertices.data(), sphere_vertices.size() * sizeof(PosColorVertex)),
    pos_col_vert_layout_);
  sphere_col_ibh_ = bgfx::createIndexBuffer(bgfx::makeRef(
    sphere_indices.data(), sphere_indices.size() * sizeof(int16_t)));

  program_col_ = createShaderProgram(
                   "shader/simple/v_simple.bin", "shader/simple/f_simple.bin")
                   .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));

  cameras_.addCamera(&first_person_rotate_camera_);
  cameras_.addCamera(&first_person_translate_camera_);

  camera_system_.cameras_ = cameras_;
}

void globe_scene_t::input(const SDL_Event& current_event)
{
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void globe_scene_t::update(debug_draw_t& debug_draw, const float delta_time)
{
  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ = asci::smoothCamera(
    camera_, target_camera_, asci::SmoothProps{}, delta_time);

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);

  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  float model[16];
  as::mat_to_arr(as::mat4::identity(), model);
  bgfx::setTransform(model);

  bgfx::setVertexBuffer(0, sphere_col_vbh_);
  bgfx::setIndexBuffer(sphere_col_ibh_);
  bgfx::setState(BGFX_STATE_DEFAULT);
  bgfx::submit(main_view_, program_col_);
}
