#pragma once

#include "scene.h"

#include "bgfx-helpers.h"

#include <as-camera-input/as-camera-input.hpp>
#include <as/as-math-ops.hpp>
#include <bgfx/bgfx.h>

#include <vector>

// notice
// the code contained in this file is derivative of the original arcball
// demo by Ken Showmake (see
// https://github.com/erich666/GraphicsGems/tree/master/gemsiv/arcball)
// the original license can be found here
// https://github.com/erich666/GraphicsGems/blob/master/LICENSE.md

struct arcball_scene_t : public scene_t {
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override;

  std::vector<PosNormalVertex> ship_vertices_;
  std::vector<uint16_t> ship_indices_;

  bgfx::VertexLayout pos_norm_vert_layout_;
  bgfx::VertexBufferHandle ship_norm_vbh_;
  bgfx::IndexBufferHandle ship_norm_ibh_;
  bgfx::ProgramHandle program_norm_;

  bgfx::UniformHandle u_light_pos_;
  bgfx::UniformHandle u_camera_pos_;
  bgfx::UniformHandle u_model_color_;
  as::vec3 light_pos_{0.0f, 8.0f, 8.0f};
  as::vec3 model_color_{0.0f, 0.9f, 0.0f};

  as::vec2i screen_dimension_;
  as::mat4 perspective_projection_;

  asc::Camera camera_;
  asc::Camera target_camera_;
  asci::CameraSystem camera_system_;
  asci::RotateCameraInput first_person_rotate_camera_{asci::MouseButton::Right};
  asci::TranslateCameraInput first_person_translate_camera_{
    asci::lookTranslation, asci::translatePivot};

  bgfx::ViewId main_view_;
  bgfx::ViewId ortho_view_;

  // arcball
  as::vec2i mouse_now_;
  as::vec2 v_now_;
  as::vec2 v_down_;
  as::quat q_target = as::quat::identity();
  as::quat q_now_ = as::quat::identity();
  as::quat q_down_ = as::quat::identity();
  as::mat3 m_now_ = as::mat3::identity();
  as::mat3 m_down_ = as::mat3::identity();
  as::vec3 v_from_;
  as::vec3 v_to_;
  bool dragging_ = false;
  bool constraint_pressed_ = false;
  std::optional<as::index> axis_index_;

  as::vec2 sphere_position_ = as::vec2::zero();
  float sphere_radius_ = 0.75f;
};
