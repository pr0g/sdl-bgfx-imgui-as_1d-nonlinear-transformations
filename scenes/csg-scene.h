#pragma once

#include "bgfx-helpers.h"
#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>
#include <as/as-math-ops.hpp>
#include <bgfx/bgfx.h>

#include <array>
#include <vector>

struct plane_t {
  as::vec3f normal;
  float w;
};

struct csg_vertex_t {
  as::vec3f pos;
  as::vec3f normal;
};

using csg_vertices_t = std::vector<csg_vertex_t>;

struct csg_polygon_t {
  csg_vertices_t vertices;
  plane_t plane;
};

using csg_polygons_t = std::vector<csg_polygon_t>;

struct csg_t {
  csg_polygons_t polygons;
};

struct csg_node_t {
  std::optional<plane_t> plane;
  csg_node_t* front = nullptr;
  csg_node_t* back = nullptr;
  csg_polygons_t polygons;
};

inline csg_t csg_from_polygons(csg_polygons_t polygons) {
  return csg_t{.polygons = std::move(polygons)};
}

inline plane_t csg_plane_from_points(
  const as::vec3f& a, const as::vec3f& b, const as::vec3f& c) {
  const auto n = as::vec_normalize(as::vec3_cross(b - a, c - a));
  return {n, as::vec_dot(n, a)};
}

inline plane_t csg_flip_plane(const plane_t& plane) {
  return {-plane.normal, -plane.w};
}

inline csg_vertex_t csg_interpolate(
  const csg_vertex_t& lhs, const csg_vertex_t& rhs, float t) {
  return {
    as::vec_mix(lhs.pos, rhs.pos, t), as::vec_mix(lhs.normal, rhs.normal, t)};
}

csg_polygon_t csg_flip_polygon(const csg_polygon_t& polygon);

csg_polygon_t csg_polygon_from_vertices(const csg_vertices_t& vertices);

void csg_split_polygon_by_plane(
  const csg_polygon_t& polygon_t, const plane_t& plane,
  csg_polygons_t& coplanarFront, csg_polygons_t& coplanarBack,
  csg_polygons_t& front, csg_polygons_t& back);

void csg_invert(csg_node_t& node);

csg_polygons_t csg_clip_polygons(
  const csg_node_t& node, const csg_polygons_t& polygons);

void csg_clip_to(csg_node_t& node_lhs, csg_node_t& node_rhs);

csg_polygons_t csg_all_polygons(const csg_node_t& node);

void csg_build_node(csg_node_t& node, const csg_polygons_t& polygons);

csg_t csg_union(const csg_t& lhs, const csg_t& rhs);
csg_t csg_subtract(const csg_t& lhs, const csg_t& rhs);
csg_t csg_intersect(const csg_t& lhs, const csg_t& rhs);

// shapes

csg_t csg_cube(const as::vec3f& position, const as::vec3f& radius);
csg_t csg_sphere(const as::vec3f& position, float radius);
csg_t csg_cylinder(
  const as::vec3f& start = -as::vec3f::axis_y(),
  const as::vec3f& end = as::vec3f::axis_y(), float radius = 1.0f);

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
  asci::RotateCameraInput orbit_rotate_camera_{asci::MouseButton::Left};
  asci::PivotDollyScrollCameraInput orbit_dolly_camera_{};

  bgfx::ViewId main_view_;
  bgfx::ViewId ortho_view_;

  std::vector<PosNormalVertex> csg_vertices_;
  std::vector<uint16_t> csg_indices_;

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
