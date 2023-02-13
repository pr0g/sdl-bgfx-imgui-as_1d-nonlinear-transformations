#include "scene.h"

#include "bgfx-helpers.h"

#include <as-camera-input/as-camera-input.hpp>
#include <as/as-math-ops.hpp>
#include <bgfx/bgfx.h>

#include <vector>

struct arcball_scene_t : public scene_t
{
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override {}

  std::vector<PosColorVertex> ship_vertices_;
  std::vector<uint16_t> ship_indices_;

  bgfx::VertexLayout pos_col_vert_layout_;
  bgfx::VertexBufferHandle ship_col_vbh_;
  bgfx::IndexBufferHandle ship_col_ibh_;
  bgfx::ProgramHandle program_col_;

  as::mat4 perspective_projection_;
  asc::Camera camera_;

  bgfx::ViewId main_view_;
};
