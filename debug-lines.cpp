#include "debug-lines.h"

namespace dbg
{

void DebugVertex::init()
{
  Layout.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();
}

const DebugVertex DebugQuads::QuadVertices[] = {
  {as::vec3{-0.5f, -0.5f, 0.0f}, 0xffffffff},
  {as::vec3{0.5f, -0.5f, 0.0}, 0xffffffff},
  {as::vec3{0.5f, 0.5f, 0.0f}, 0xffffffff},
  {as::vec3{-0.5f, 0.5f, 0.0f}, 0xffffffff}};

const uint16_t DebugQuads::QuadIndices[] = {0, 1, 2, 0, 2, 3};

DebugQuads::DebugQuads(
  const bgfx::ViewId view, const bgfx::ProgramHandle program_handle)
  : view_(view), program_handle_(program_handle)
{
  quad_vbh_ = bgfx::createVertexBuffer(
    bgfx::makeRef(QuadVertices, sizeof(QuadVertices)), DebugVertex::Layout);
  quad_ibh_ =
    bgfx::createIndexBuffer(bgfx::makeRef(QuadIndices, sizeof(QuadIndices)));
}

DebugQuads::~DebugQuads()
{
  bgfx::destroy(quad_vbh_);
  bgfx::destroy(quad_ibh_);
}

void DebugQuads::addQuad(
  const as::mat4& transform, const as::vec4& color)
{
  instances_.emplace_back(transform, color);
}

void DebugQuads::submit()
{
  // 80 bytes stride = 64 bytes for mat4 + 16 bytes for RGBA color.
  const uint16_t instance_stride = 80;

  if (
    bgfx::getAvailInstanceDataBuffer(instances_.size(), instance_stride)
    == instances_.size()) {
    bgfx::InstanceDataBuffer idb;
    bgfx::allocInstanceDataBuffer(&idb, instances_.size(), instance_stride);

    uint8_t* data = idb.data;
    for (const auto& quad_instance : instances_) {
      auto* inst_transform = (float*)data;
      std::copy(
        as::mat_const_data(quad_instance.transform_),
        as::mat_const_data(quad_instance.transform_) + 16, inst_transform);

      auto* color = (float*)&data[64];
      color[0] = quad_instance.color_[0];
      color[1] = quad_instance.color_[1];
      color[2] = quad_instance.color_[2];
      color[3] = quad_instance.color_[3];

      data += instance_stride;
    }

    bgfx::setVertexBuffer(view_, quad_vbh_);
    bgfx::setIndexBuffer(quad_ibh_);

    bgfx::setInstanceDataBuffer(&idb);

    bgfx::setState(BGFX_STATE_DEFAULT);
    bgfx::submit(view_, program_handle_);
  }
}

void DebugLines::addLine(
  const as::vec3& begin, const as::vec3& end, const uint32_t col)
{
  lines_.emplace_back(begin, end, col);
}

void DebugLines::submit()
{
  const auto requested_vertex_count = lines_.size() * 2;
  if (requested_vertex_count == 0) {
    return;
  }

  const auto available_vertex_count = bgfx::getAvailTransientVertexBuffer(
    requested_vertex_count, DebugVertex::Layout);
  if (available_vertex_count != requested_vertex_count) {
    return;
  }

  bgfx::TransientVertexBuffer line_tvb;
  bgfx::allocTransientVertexBuffer(
    &line_tvb, requested_vertex_count, DebugVertex::Layout);

  std::memcpy(
    line_tvb.data, lines_.data(),
    DebugVertex::Layout.getStride() * requested_vertex_count);

  float transform[16];
  as::mat_to_arr(transform_, transform);
  bgfx::setTransform(transform);

  bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);

  bgfx::setVertexBuffer(0, &line_tvb, 0, requested_vertex_count);
  bgfx::submit(view_, program_handle_);
}

} // namespace dbg
