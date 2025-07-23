#include "csg.h"

#include "hash-combine.h"

#include <bec/bitfield-enum-class.hpp>

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
  csg_polygons_t& coplanarFront, csg_polygons_t& coplanarBack,
  csg_polygons_t& front, csg_polygons_t& back) {

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
      ((as::vec_dot(plane.normal, polygon.plane.normal) > 0.0f) ? coplanarFront
                                                                : coplanarBack)
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
  flipped_polygon.plane = csg_flip_plane(polygon.plane);
  return flipped_polygon;
}

csg_polygon_t csg_polygon_from_vertices(const csg_vertices_t& vertices) {
  return csg_polygon_t{
    .vertices = vertices,
    .plane =
      csg_plane_from_points(vertices[0].pos, vertices[1].pos, vertices[2].pos)};
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
  node.plane = csg_flip_plane(*node.plane);
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
  return csg_from_polygons(csg_all_polygons(a));
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
  return csg_from_polygons(csg_all_polygons(a));
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
  return csg_from_polygons(csg_all_polygons(a));
}

// shapes

csg_t csg_cube(const as::vec3f& center, const as::vec3f& radius) {
  using info_t = std::pair<std::array<int, 4>, as::vec3f>;
  std::vector<info_t> info = {
    {{0, 4, 6, 2}, {-1.0f, 0.0f, 0.0f}}, {{1, 3, 7, 5}, {1.0f, 0.0f, 0.0f}},
    {{0, 1, 5, 4}, {0.0f, -1.0f, 0.0f}}, {{2, 6, 7, 3}, {0.0f, 1.0f, 0.0f}},
    {{0, 2, 3, 1}, {0.0f, 0.0f, -1.0f}}, {{4, 5, 7, 6}, {0.0f, 0.0f, 1.0f}}};
  csg_polygons_t polygons;
  std::transform(
    info.begin(), info.end(), std::back_inserter(polygons),
    [&center, &radius](const info_t& info) {
      csg_vertices_t vertices;
      std::transform(
        info.first.begin(), info.first.end(), std::back_inserter(vertices),
        [&center, &radius, &info](const int i) {
          const auto pos = as::vec3f(
            center.x + radius.x * (2.0f * !!(i & 1) - 1.0f),
            center.y + radius.y * (2.0f * !!(i & 2) - 1.0f),
            center.z + radius.z * (2.0f * !!(i & 4) - 1.0f));
          return csg_vertex_t{.pos = pos, .normal = info.second};
        });
      return csg_polygon_from_vertices(vertices);
    });
  return csg_t{.polygons = polygons};
}

csg_t csg_sphere(const as::vec3f& center, float radius) {
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
  const int slices = 16;
  const int stacks = 8;
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
  return csg_from_polygons(std::move(polygons));
}

csg_t csg_cylinder(const as::vec3f& start, const as::vec3f& end, float radius) {
  const as::vec3f ray = end - start;
  int slices = 16;
  const auto axis_z = as::vec_normalize(ray);
  const bool is_y = std::abs(axis_z.y) > 0.5f;
  const auto axis_x = as::vec_normalize(
    as::vec3_cross(as::vec3f((float)!!is_y, (float)!!!is_y, 0.0f), axis_z));
  const auto axis_y = as::vec3_cross(axis_x, axis_z);
  const auto start_vertex = csg_vertex_t{.pos = start, .normal = -axis_z};
  const auto end_vertex = csg_vertex_t{.pos = end, .normal = axis_z};
  csg_polygons_t polygons;
  const auto point = [&axis_x, &axis_y, &axis_z, &start, &ray,
                      radius](float stack, float slice, float normal_blend) {
    const auto angle = slice * as::k_two_pi;
    const auto out = (axis_x * std::cos(angle)) + (axis_y * std::sin(angle));
    const auto pos = (start + ray * stack) + out * radius;
    const auto normal =
      (out * (1.0f - std::abs(normal_blend))) + (axis_z * normal_blend);
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
  return csg_from_polygons(std::move(polygons));
}
