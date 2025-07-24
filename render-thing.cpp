#include "render-thing.h"

#include "csg/csg.h"

#include <thh-bgfx-debug/debug-line.hpp>

bgfx::VertexLayout render_thing_t::vertex_layout_;
bgfx::ProgramHandle render_thing_t::program_;

static void build_mesh_from_csg(
  const csg_t& csg, std::vector<PosNormalVertex>& csg_vertices,
  std::vector<uint16_t>& csg_indices) {
  std::unordered_map<csg_vertex_t, int, csg_vertex_hash_t, csg_vertex_equals_t>
    indexer;
  for (const csg_polygon_t& polygon : csg.polygons) {
    std::vector<int> indices;
    for (const csg_vertex_t vertex : polygon.vertices) {
      if (const auto index = indexer.find(vertex); index == indexer.end()) {
        indices.push_back(indexer.size());
        indexer.insert({vertex, indexer.size()});
        csg_vertices.push_back(
          PosNormalVertex{.position_ = vertex.pos, .normal_ = vertex.normal});
      } else {
        indices.push_back(index->second);
      }
    }
    for (int i = 2; i < indices.size(); i++) {
      csg_indices.push_back(indices[0]);
      csg_indices.push_back(indices[i - 1]);
      csg_indices.push_back(indices[i]);
    }
  }
}

void render_thing_t::init() {
  vertex_layout_.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
    .end();

  program_ =
    createShaderProgram(
      "shader/basic-lighting/v_basic.bin", "shader/basic-lighting/f_basic.bin")
      .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));

  if (!bgfx::isValid(program_)) {
    std::terminate();
  }
}

void render_thing_t::deinit() {
  bgfx::destroy(render_thing_t::program_);
}

render_thing_t render_thing_from_csg(const csg_t& csg, const as::vec3f& color) {
  render_thing_t render_thing;
  render_thing.color_ = color;
  build_mesh_from_csg(csg, render_thing.vertices_, render_thing.indices_);
  render_thing.norm_vbh_ = bgfx::createVertexBuffer(
    bgfx::makeRef(
      render_thing.vertices_.data(),
      render_thing.vertices_.size() * sizeof(PosNormalVertex)),
    render_thing_t::vertex_layout_);
  render_thing.norm_ibh_ = bgfx::createIndexBuffer(
    bgfx::makeRef(
      render_thing.indices_.data(),
      render_thing.indices_.size() * sizeof(uint16_t)));
  render_thing.color_uniform_ =
    bgfx::createUniform("u_modelColor", bgfx::UniformType::Vec4, 1);
  return render_thing;
}

void render_thing_draw(
  const render_thing_t& render_thing, const bgfx::ViewId view) {
  bgfx::setUniform(render_thing.color_uniform_, (void*)&render_thing.color_, 1);
  bgfx::setVertexBuffer(0, render_thing.norm_vbh_);
  bgfx::setIndexBuffer(render_thing.norm_ibh_);
  bgfx::setState(
    BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z
    | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_CULL_CCW);
  bgfx::submit(view, render_thing_t::program_);
}

void render_thing_debug(
  const render_thing_t& render_thing, dbg::DebugLines& debug_lines) {
  for (const auto index : render_thing.indices_) {
    // normals
    debug_lines.addLine(
      render_thing.vertices_[index].position_,
      render_thing.vertices_[index].position_
        + render_thing.vertices_[index].normal_ * 0.5f,
      0xff55dd55);
  }

  const float adjustment = 0.005f;
  for (int index = 2; index < render_thing.indices_.size(); index += 3) {
    // triangles (lines)
    debug_lines.addLine(
      render_thing.vertices_[render_thing.indices_[index - 2]].position_
        + render_thing.vertices_[render_thing.indices_[index - 2]].normal_
            * adjustment,
      render_thing.vertices_[render_thing.indices_[index - 1]].position_
        + render_thing.vertices_[render_thing.indices_[index - 1]].normal_
            * adjustment,
      0xffaaaaaa);
    debug_lines.addLine(
      render_thing.vertices_[render_thing.indices_[index - 1]].position_
        + render_thing.vertices_[render_thing.indices_[index - 1]].normal_
            * adjustment,
      render_thing.vertices_[render_thing.indices_[index]].position_
        + render_thing.vertices_[render_thing.indices_[index]].normal_
            * adjustment,
      0xffaaaaaa);
    debug_lines.addLine(
      render_thing.vertices_[render_thing.indices_[index]].position_
        + render_thing.vertices_[render_thing.indices_[index]].normal_
            * adjustment,
      render_thing.vertices_[render_thing.indices_[index - 2]].position_
        + render_thing.vertices_[render_thing.indices_[index - 2]].normal_
            * adjustment,
      0xffaaaaaa);
  }
}

void destroy_render_thing(const render_thing_t& render_thing) {
  bgfx::destroy(render_thing.color_uniform_);
  bgfx::destroy(render_thing.norm_ibh_);
  bgfx::destroy(render_thing.norm_vbh_);
}
