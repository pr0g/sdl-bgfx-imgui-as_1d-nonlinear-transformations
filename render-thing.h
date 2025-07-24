#pragma once

#include "bgfx-helpers.h"

#include <bgfx/bgfx.h>

#include <vector>

struct csg_t;

struct render_thing_t {
  std::vector<PosNormalVertex> csg_vertices_;
  std::vector<uint16_t> indices_;
  bgfx::VertexBufferHandle norm_vbh_;
  bgfx::IndexBufferHandle norm_ibh_;
};

render_thing_t render_thing_from_csg(const csg_t& csg);
