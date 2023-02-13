#include "arcball-scene.h"

#include <as/as-view.hpp>

void arcball_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  pos_col_vert_layout_.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();

  main_view_ = main_view;

  perspective_projection_ = as::perspective_direct3d_lh(
    as::radians(60.0f), float(width) / float(height), 0.01f, 1000.0f);

  ship_vertices_ = {
    {as::vec3{3.0f, 0.0f, 0.0f}, 0xffffffff},
    {as::vec3{-1.0f, 1.0f, 0.0f}, 0xffffffff},
    {as::vec3{-1.0f, -1.0f, 0.0f}, 0xffffffff},
    {as::vec3{-0.75f, 0.0f, -0.25f}, 0xffffffff},
    {as::vec3{1.f, 0.0f, 0.0f}, 0xffffffff},
    {as::vec3{-0.75, 0.0f, 0.75f}, 0xffffffff},
    {as::vec3{-0.5, -0.125, 0.0f}, 0xffffffff},
    {as::vec3{-0.5, 0.125, 0.0f}, 0xffffffff},
  };

  ship_indices_ = {0, 1, 2, 4, 5, 6, 4, 7, 5, 5, 7,
                   6, 0, 2, 3, 0, 3, 1, 1, 3, 2};

  ship_col_vbh_ = bgfx::createVertexBuffer(
    bgfx::makeRef(
      ship_vertices_.data(), ship_vertices_.size() * sizeof(PosColorVertex)),
    pos_col_vert_layout_);
  ship_col_ibh_ = bgfx::createIndexBuffer(bgfx::makeRef(
    ship_indices_.data(), ship_indices_.size() * sizeof(int16_t)));

  program_col_ = createShaderProgram(
                   "shader/simple/v_simple.bin", "shader/simple/f_simple.bin")
                   .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));
}

void arcball_scene_t::input(const SDL_Event& current_event)
{
}

void arcball_scene_t::update(debug_draw_t& debug_draw, const float delta_time)
{
  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  float model[16];
  as::mat_to_arr(
    as::mat4_from_mat3_vec3(
      as::mat3_rotation_x(-as::k_half_pi), as::vec3::axis_z(10.0f)),
    model);

  bgfx::setTransform(model);

  bgfx::setVertexBuffer(0, ship_col_vbh_);
  bgfx::setIndexBuffer(ship_col_ibh_);
  bgfx::setState(BGFX_STATE_DEFAULT);
  bgfx::submit(main_view_, program_col_);
}
