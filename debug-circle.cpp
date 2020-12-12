#include "debug-circle.h"

namespace dbg
{

static constexpr int CircleVerticesCount = 20;
static constexpr int CircleIndicesCount = 21;

DebugVertex DebugCircles::CircleVertices[CircleVerticesCount];
uint16_t DebugCircles::CircleIndices[CircleIndicesCount];

void DebugCircles::init()
{
  float rot = 0.0f;
  const float increment = as::k_tau / static_cast<float>(CircleVerticesCount);
  for (size_t i = 0; i < CircleVerticesCount; ++i) {
    CircleVertices[i] = {
      as::vec3(std::cos(rot), std::sin(rot), 0.0f), 0xff000000};
    CircleIndices[i] = i;
    rot += increment;
  }

  CircleIndices[CircleIndicesCount - 1] = 0;
}

DebugCircles::DebugCircles(
  const bgfx::ViewId view, const bgfx::ProgramHandle program_handle)
  : view_(view), program_handle_(program_handle)
{
  circle_vbh_ = bgfx::createVertexBuffer(
    bgfx::makeRef(CircleVertices, sizeof(CircleVertices)), DebugVertex::Layout);
  circle_ibh_ = bgfx::createIndexBuffer(
    bgfx::makeRef(CircleIndices, sizeof(CircleIndices)));
}

DebugCircles::~DebugCircles()
{
  bgfx::destroy(circle_vbh_);
  bgfx::destroy(circle_ibh_);
}

void DebugCircles::reserveCircles(const size_t count)
{
  instances_.reserve(count);
}

void DebugCircles::submit()
{
  // 80 bytes stride = 64 bytes for mat4 + 16 bytes for RGBA color.
  const uint16_t instance_stride = 80;

  if (
    !instances_.empty()
    && bgfx::getAvailInstanceDataBuffer(instances_.size(), instance_stride)
         == instances_.size()) {
    bgfx::InstanceDataBuffer idb;
    bgfx::allocInstanceDataBuffer(&idb, instances_.size(), instance_stride);

    uint8_t* data = idb.data;
    for (const auto& circle_instance : instances_) {
      auto* inst_transform = (float*)data;
      std::copy(
        as::mat_const_data(circle_instance.transform_),
        as::mat_const_data(circle_instance.transform_) + 16, inst_transform);

      auto* color = (float*)&data[64];
      color[0] = circle_instance.color_[0];
      color[1] = circle_instance.color_[1];
      color[2] = circle_instance.color_[2];
      color[3] = circle_instance.color_[3];

      data += instance_stride;
    }

    bgfx::setVertexBuffer(view_, circle_vbh_);
    bgfx::setIndexBuffer(circle_ibh_);

    bgfx::setInstanceDataBuffer(&idb);

    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINESTRIP);
    bgfx::submit(view_, program_handle_);
  }
}

DebugSpheres::DebugSpheres(DebugCircles& debug_circles)
  : debug_circles_(debug_circles)
{
}

void DebugSpheres::addSphere(const as::mat4& transform, const as::vec4& color)
{
  constexpr size_t Loops = 8;
  const float vertical_angle_inc_rad = as::radians(180.0f / float(Loops));
  const float starting_vertical_angle_rad = as::radians(90.0f);

  float current_vertical_angle_rad =
    starting_vertical_angle_rad + vertical_angle_inc_rad;
  for (size_t loop = 0; loop < Loops - 1; ++loop) {
    const float vertical_position = std::sin(current_vertical_angle_rad);
    const float horizontal_scale = std::cos(current_vertical_angle_rad);

    const auto translation = as::mat4_from_mat3_vec3(
      as::mat3_rotation_x(as::radians(90.0f)),
      as::vec3::axis_y(vertical_position));
    const auto scale = as::mat4_from_mat3(as::mat3_scale(horizontal_scale));

    debug_circles_.addCircle(
      as::mat_mul(as::mat_mul(scale, translation), transform),
      as::vec4(as::vec3::zero(), 1.0f));

    current_vertical_angle_rad += vertical_angle_inc_rad;
  }
}

} // namespace dbg
