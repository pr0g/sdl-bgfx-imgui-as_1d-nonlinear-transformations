#pragma once

#include "bgfx-helpers.h"
#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>

#include <bgfx/bgfx.h>

struct line_indices_t {
  int begin;
  int end;
};
using lines_indices_t = std::vector<line_indices_t>;

struct csg_scene_t : public scene_t {
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override;

  as::vec2i screen_dimension_;
  as::mat4 perspective_projection_;

  asc::Camera camera_;
  asc::Camera target_camera_;
  asci::CameraSystem camera_system_;
  asci::RotateCameraInput first_person_rotate_camera_{asci::MouseButton::Left};
  asci::TranslateCameraInput first_person_translate_camera_{
    asci::lookTranslation, asci::translatePivot};

  bgfx::ViewId main_view_;
  bgfx::ViewId ortho_view_;

  std::vector<PosNormalVertex> csg_vertices_;
  std::vector<uint16_t> csg_indices_;
  lines_indices_t csg_lines_;

  bgfx::VertexLayout pos_norm_vert_layout_;
  bgfx::VertexBufferHandle csg_norm_vbh_;
  bgfx::IndexBufferHandle csg_norm_ibh_;
  bgfx::ProgramHandle program_norm_;

  bgfx::UniformHandle u_light_pos_;
  bgfx::UniformHandle u_camera_pos_;
  bgfx::UniformHandle u_model_color_;
  as::vec3 light_pos_{0.0f, 8.0f, 8.0f};
  as::vec3 model_color_{1.0f, 1.0f, 0.0f};
};
