#include "csg.h"

#include "hash-combine.h"

#include <bec/bitfield-enum-class.hpp>

// @evanw (@thh edit)
// `g_plane_epsilon` is the tolerance used by `csg_split_polygon_by_plane()` to
// decide if a point is on the plane.
static constexpr float g_plane_epsilon = 1e-5f;

std::size_t csg_vertex_hash_t::operator()(
  const csg_vertex_t& vertex) const noexcept {
  size_t seed = 0;
  hash_combine(seed, vertex.pos.x);
  hash_combine(seed, vertex.pos.y);
  hash_combine(seed, vertex.pos.z);
  hash_combine(seed, vertex.normal.x);
  hash_combine(seed, vertex.normal.y);
  hash_combine(seed, vertex.normal.z);
  return seed;
}

enum class polygon_type_e { coplanar, front, back, spanning };
template<>
struct bec::EnableBitMaskOperators<polygon_type_e> {
  static const bool Enable = true;
};

void csg_split_polygon_by_plane(
  const csg_polygon_t& polygon, const plane_t& plane,
  csg_polygons_t& coplanar_front, csg_polygons_t& coplanar_back,
  csg_polygons_t& front, csg_polygons_t& back) {

  // classify each point as well as the entire polygon into one of the above
  // four classes.
  using bec::operator|=;
  using bec::operator|;
  polygon_type_e polygon_type = polygon_type_e::coplanar;
  std::vector<polygon_type_e> types;
  for (const auto vertex : polygon.vertices) {
    const float t = as::vec_dot(plane.normal, vertex.pos) - plane.w;
    const polygon_type_e type = (t < -g_plane_epsilon) ? polygon_type_e::back
                              : (t > g_plane_epsilon)
                                ? polygon_type_e::front
                                : polygon_type_e::coplanar;
    polygon_type |= type;
    types.push_back(type);
  }

  // put the polygon in the correct list, splitting when necessary
  switch (polygon_type) {
    case polygon_type_e::coplanar:
      ((as::vec_dot(plane.normal, polygon.plane.normal) > 0.0f) ? coplanar_front
                                                                : coplanar_back)
        .push_back(polygon);
      break;
    case polygon_type_e::front:
      front.push_back(polygon);
      break;
    case polygon_type_e::back:
      back.push_back(polygon);
      break;
    case polygon_type_e::spanning: {
      csg_vertices_t f;
      csg_vertices_t b;
      for (int i = 0; i < polygon.vertices.size(); i++) {
        const int j = (i + 1) % polygon.vertices.size();
        const polygon_type_e ti = types[i];
        const polygon_type_e tj = types[j];
        const csg_vertex_t vi = polygon.vertices[i];
        const csg_vertex_t vj = polygon.vertices[j];
        if (ti != polygon_type_e::back) {
          f.push_back(vi);
        }
        if (ti != polygon_type_e::front) {
          b.push_back(vi);
        }
        if ((ti | tj) == polygon_type_e::spanning) {
          const auto t = (plane.w - as::vec_dot(plane.normal, vi.pos))
                       / as::vec_dot(plane.normal, vj.pos - vi.pos);
          const auto v = csg_interpolate(vi, vj, t);
          f.push_back(v);
          b.push_back(v);
        }
      }
      if (f.size() >= 3) {
        front.push_back(csg_polygon_from_vertices(f));
      }
      if (b.size() >= 3) {
        back.push_back(csg_polygon_from_vertices(b));
      }
    } break;
  }
}

csg_polygon_t csg_flip_polygon(const csg_polygon_t& polygon) {
  csg_polygon_t flipped_polygon;
  flipped_polygon.vertices = polygon.vertices;
  std::reverse(
    flipped_polygon.vertices.begin(), flipped_polygon.vertices.end());
  std::for_each(
    flipped_polygon.vertices.begin(), flipped_polygon.vertices.end(),
    [](csg_vertex_t& v) { v.normal = -v.normal; });
  flipped_polygon.plane = flip_plane(polygon.plane);
  return flipped_polygon;
}

csg_polygon_t csg_polygon_from_vertices(const csg_vertices_t& vertices) {
  return csg_polygon_t{
    .vertices = vertices,
    .plane =
      plane_from_points(vertices[0].pos, vertices[1].pos, vertices[2].pos)};
}

csg_polygons_t csg_clip_polygons(
  const csg_node_t& node, const csg_polygons_t& polygons) {
  if (!node.plane.has_value()) {
    return polygons;
  }
  csg_polygons_t front, back;
  for (int i = 0; i < polygons.size(); i++) {
    csg_split_polygon_by_plane(
      polygons[i], *node.plane, front, back, front, back);
  }
  if (node.front) {
    front = csg_clip_polygons(*node.front, front);
  }
  if (node.back) {
    back = csg_clip_polygons(*node.back, back);
  } else {
    back.clear();
  }
  front.insert(front.end(), back.begin(), back.end());
  return front;
}

void csg_clip_to(csg_node_t& node_lhs, csg_node_t& node_rhs) {
  node_lhs.polygons = csg_clip_polygons(node_rhs, node_lhs.polygons);
  if (node_lhs.front) {
    csg_clip_to(*node_lhs.front, node_rhs);
  }
  if (node_lhs.back) {
    csg_clip_to(*node_lhs.back, node_rhs);
  }
}

csg_polygons_t csg_all_polygons(const csg_node_t& node) {
  csg_polygons_t polygons = node.polygons;
  if (node.front) {
    const auto front_polygons = csg_all_polygons(*node.front);
    polygons.insert(
      polygons.end(), front_polygons.begin(), front_polygons.end());
  }
  if (node.back) {
    const auto back_polygons = csg_all_polygons(*node.back);
    polygons.insert(polygons.end(), back_polygons.begin(), back_polygons.end());
  }
  return polygons;
}

void csg_build_node(csg_node_t& node, const csg_polygons_t& polygons) {
  if (polygons.empty()) {
    return;
  }
  if (!node.plane.has_value()) {
    node.plane = polygons[0].plane;
  }
  csg_polygons_t front;
  csg_polygons_t back;
  for (int i = 0; i < polygons.size(); i++) {
    csg_split_polygon_by_plane(
      polygons[i], *node.plane, node.polygons, node.polygons, front, back);
  }
  if (!front.empty()) {
    if (!node.front) {
      node.front = std::make_unique<csg_node_t>();
    }
    csg_build_node(*node.front, front);
  }
  if (!back.empty()) {
    if (!node.back) {
      node.back = std::make_unique<csg_node_t>();
    }
    csg_build_node(*node.back, back);
  }
}

void csg_invert(csg_node_t& node) {
  for (int i = 0; i < node.polygons.size(); i++) {
    node.polygons[i] = csg_flip_polygon(node.polygons[i]);
  }
  node.plane = flip_plane(*node.plane);
  if (node.front) {
    csg_invert(*node.front);
  }
  if (node.back) {
    csg_invert(*node.back);
  }
  std::swap(node.front, node.back);
}

csg_t csg_union(const csg_t& lhs, const csg_t& rhs) {
  csg_node_t a, b;
  csg_build_node(a, lhs.polygons);
  csg_build_node(b, rhs.polygons);
  csg_clip_to(a, b);
  csg_clip_to(b, a);
  csg_invert(b);
  csg_clip_to(b, a);
  csg_invert(b);
  csg_build_node(a, csg_all_polygons(b));
  return csg_t{.polygons = csg_all_polygons(a)};
}

csg_t csg_subtract(const csg_t& lhs, const csg_t& rhs) {
  csg_node_t a, b;
  csg_build_node(a, lhs.polygons);
  csg_build_node(b, rhs.polygons);
  csg_invert(a);
  csg_clip_to(a, b);
  csg_clip_to(b, a);
  csg_invert(b);
  csg_clip_to(b, a);
  csg_invert(b);
  csg_build_node(a, csg_all_polygons(b));
  csg_invert(a);
  return csg_t{.polygons = csg_all_polygons(a)};
}

csg_t csg_intersect(const csg_t& lhs, const csg_t& rhs) {
  csg_node_t a, b;
  csg_build_node(a, lhs.polygons);
  csg_build_node(b, rhs.polygons);
  csg_invert(a);
  csg_clip_to(b, a);
  csg_invert(b);
  csg_clip_to(a, b);
  csg_clip_to(b, a);
  csg_build_node(a, csg_all_polygons(b));
  csg_invert(a);
  return csg_t{.polygons = csg_all_polygons(a)};
}

csg_t csg_inverse(const csg_t& csg) {
  csg_t inverted_csg;
  inverted_csg.polygons.resize(csg.polygons.size());
  std::transform(
    csg.polygons.begin(), csg.polygons.end(), inverted_csg.polygons.begin(),
    [](const csg_polygon_t& polygon) { return csg_flip_polygon(polygon); });
  return inverted_csg;
}

// shapes

csg_t csg_cube(const csg_cube_config_t& config) {
  auto [min, max, transform] = config;
  const std::array<as::vec3f, 8> corners = {{
    // b - bottom, t - top, n - near, f - far, l - left, r - right
    {min}, // bnl
    {max.x, min.y, min.z}, // bnr
    {min.x, max.y, min.z}, // tnl
    {max.x, max.y, min.z}, // tnr
    {max}, // tfr
    {max.x, min.y, max.z}, // bfr
    {min.x, max.y, max.z}, // tfl
    {min.x, min.y, max.z}, // bfl
  }};
  using info_t = std::pair<std::array<int, 4>, as::vec3f>;
  std::vector<info_t> info = {
    {{0, 7, 6, 2}, {-1.0f, 0.0f, 0.0f}}, {{1, 3, 4, 5}, {1.0f, 0.0f, 0.0f}},
    {{0, 1, 5, 7}, {0.0f, -1.0f, 0.0f}}, {{2, 6, 4, 3}, {0.0f, 1.0f, 0.0f}},
    {{0, 2, 3, 1}, {0.0f, 0.0f, -1.0f}}, {{4, 6, 7, 5}, {0.0f, 0.0f, 1.0f}}};
  csg_polygons_t polygons;
  std::transform(
    info.begin(), info.end(), std::back_inserter(polygons),
    [&corners, &transform](const info_t& info) {
      csg_vertices_t vertices;
      std::transform(
        info.first.begin(), info.first.end(), std::back_inserter(vertices),
        [&info, &corners, &transform](const int i) {
          return csg_vertex_t{
            .pos = as::affine_transform_pos(transform, corners[i]),
            .normal = as::affine_transform_dir(transform, info.second)};
        });
      return csg_polygon_from_vertices(vertices);
    });
  return csg_t{.polygons = polygons};
}

csg_t csg_sphere(const csg_sphere_config_t& config) {
  const auto& [center, radius, slices, stacks] = config;
  csg_polygons_t polygons;
  const auto vertex =
    [&center,
     radius](const float theta, const float phi, csg_vertices_t& vertices) {
      const float theta_rad = theta * as::k_two_pi;
      const float phi_rad = phi * as::k_pi;
      const as::vec3f dir = as::vec3f(
        std::cos(theta_rad) * std::sin(phi_rad), std::cos(phi_rad),
        std::sin(theta_rad) * std::sin(phi_rad));
      vertices.push_back(
        csg_vertex_t{.pos = center + dir * radius, .normal = dir});
    };
  for (int i = 0; i < slices; i++) {
    for (int j = 0; j < stacks; j++) {
      csg_vertices_t vertices;
      vertex(i / (float)slices, j / (float)stacks, vertices);
      if (j > 0) {
        vertex((i + 1) / (float)slices, j / (float)stacks, vertices);
      }
      if (j < stacks - 1) {
        vertex((i + 1) / (float)slices, (j + 1) / (float)stacks, vertices);
      }
      vertex(i / (float)slices, (j + 1) / (float)stacks, vertices);
      polygons.push_back(csg_polygon_from_vertices(vertices));
    }
  }
  return csg_t{.polygons = std::move(polygons)};
}

csg_t csg_cylinder(const csg_cylinder_config_t& config) {
  const auto& [start, end, radius, slices] = config;
  const as::vec3f ray = end - start;
  const as::mat3f basis = as::orthonormal_basis(as::vec_normalize(ray));
  // direction of cylinder
  const as::vec3f main_axis = as::mat3_basis_x(basis);
  // axes orthogonal to main_axis, and each other
  const as::vec3f side_axis_1 = as::mat3_basis_y(basis);
  const as::vec3f side_axis_2 = as::mat3_basis_z(basis);
  const auto start_vertex = csg_vertex_t{.pos = start, .normal = -main_axis};
  const auto end_vertex = csg_vertex_t{.pos = end, .normal = main_axis};
  csg_polygons_t polygons;
  const auto point = [&side_axis_2, &side_axis_1, &main_axis, &start, &ray,
                      radius](float stack, float slice, float normal_blend) {
    const auto angle = slice * as::k_two_pi;
    const auto out =
      side_axis_2 * std::cos(angle) + side_axis_1 * std::sin(angle);
    const auto pos = (start + ray * stack) + out * radius;
    const auto normal =
      (out * (1.0f - std::abs(normal_blend))) + (main_axis * normal_blend);
    return csg_vertex_t{.pos = pos, .normal = normal};
  };
  for (int i = 0; i < slices; i++) {
    const float t0 = i / (float)slices;
    const float t1 = (i + 1) / (float)slices;
    polygons.push_back(csg_polygon_from_vertices(
      csg_vertices_t{
        start_vertex, point(0.0f, t0, -1.0f), point(0.0f, t1, -1.0f)}));
    polygons.push_back(csg_polygon_from_vertices(
      csg_vertices_t{
        point(0.0f, t1, 0.0f), point(0.0f, t0, 0.0f), point(1.0f, t0, 0.0f),
        point(1.0f, t1, 0.0f)}));
    polygons.push_back(csg_polygon_from_vertices(
      csg_vertices_t{
        end_vertex, point(1.0f, t1, 1.0f), point(1.0f, t0, 1.0f)}));
  }
  return csg_t{.polygons = std::move(polygons)};
}

void csg_transform_csg_inplace(csg_t& csg, const as::mat4f& transform) {
  const auto inverse_transpose =
    as::mat_transpose(as::mat_inverse(as::mat3_from_mat4(transform)));
  for (auto& polygon : csg.polygons) {
    const auto point = polygon.plane.normal * polygon.plane.w;
    const auto transformed_point = transform * as::vec4_translation(point);
    const auto transformed_normal = inverse_transpose * polygon.plane.normal;
    polygon.plane.normal = transformed_normal;
    polygon.plane.w =
      as::vec_dot(as::vec3_from_vec4(transformed_point), transformed_normal);
    for (auto& vertex : polygon.vertices) {
      vertex.normal = inverse_transpose * vertex.normal;
      vertex.pos =
        as::vec3_from_vec4(transform * as::vec4_translation(vertex.pos));
    }
  }
}

csg_t csg_transform_csg(const csg_t& csg, const as::mat4f& transform) {
  csg_t transformed_csg = csg;
  csg_transform_csg_inplace(transformed_csg, transform);
  return transformed_csg;
}
