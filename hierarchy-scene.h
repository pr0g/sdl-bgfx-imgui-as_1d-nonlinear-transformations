#pragma once

#include "hierarchy-imgui.h"
#include "scene.h"

#include <bgfx/bgfx.h>
#include <thh-bgfx-debug/debug-shader.hpp>

struct imgui_hierarchy_scene_t : public scene_t
{
  void setup(uint16_t width, uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw) override;
  void teardown() override;

  uint16_t main_view() const override { return main_view_; }
  uint16_t ortho_view() const override { return ortho_view_; }
  bool quit() const override { return quit_; }

  bgfx::ProgramHandle simple_handle() const override
  {
    return simple_program.handle();
  };
  bgfx::ProgramHandle instance_handle() const override
  {
    return instance_program.handle();
  };

  bool quit_ = false;

  const bgfx::ViewId main_view_ = 0;
  const bgfx::ViewId ortho_view_ = 1;

  dbg::EmbeddedShaderProgram simple_program;
  dbg::EmbeddedShaderProgram instance_program;

  thh::container_t<hy::entity_t> entities;
  std::vector<thh::handle_t> root_handles;
  hy::interaction_t interaction;
};
