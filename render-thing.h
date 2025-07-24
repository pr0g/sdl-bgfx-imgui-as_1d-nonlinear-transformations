#pragma once

#include "bgfx-helpers.h"

#include <as/as-mat4.hpp>
#include <bgfx/bgfx.h>

#include <vector>

struct csg_t;

namespace dbg {
class DebugLines;
}

struct render_thing_t {
  std::vector<PosNormalVertex> vertices_;
  std::vector<uint16_t> indices_;
  as::vec3f color_;
  as::mat4f transform_;

  bgfx::VertexBufferHandle norm_vbh_;
  bgfx::IndexBufferHandle norm_ibh_;
  bgfx::UniformHandle color_uniform_;

  static void init();
  static void deinit();

  static bgfx::VertexLayout vertex_layout_;
  static bgfx::ProgramHandle program_;
};

render_thing_t render_thing_from_csg(
  const csg_t& csg, const as::mat4f& transform, const as::vec3f& color);

void render_thing_debug(
  const render_thing_t& render_thing, dbg::DebugLines& debug_lines);
void render_thing_draw(const render_thing_t& render_thing, bgfx::ViewId view);

void destroy_render_thing(const render_thing_t& render_thing);
