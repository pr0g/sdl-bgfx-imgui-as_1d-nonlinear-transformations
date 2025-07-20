#include "csg-scene.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <bec/bitfield-enum-class.hpp>
#include <easy_iterator.h>
#include <imgui.h>

#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>

static constexpr float g_plane_epsilon = 1e-5f;

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
  polygon_type_e polygon_type;
  std::vector<polygon_type_e> types;
  for (const auto vertex : polygon.vertices) {
    const float t = as::vec_dot(plane.normal, vertex.pos) - plane.w;
    const polygon_type_e type = (t < g_plane_epsilon) ? polygon_type_e::back
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
      std::vector<csg_vertex_t> f;
      std::vector<csg_vertex_t> b;
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

csg_polygon_t csg_polygon_from_vertices(
  const std::vector<csg_vertex_t>& vertices) {
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
  csg_node_t csg_node;
  csg_node.plane = polygons[0].plane;
  csg_polygons_t front;
  csg_polygons_t back;
  for (int i = 0; i < polygons.size(); i++) {
    csg_split_polygon_by_plane(
      polygons[i], *node.plane, node.polygons, node.polygons, front, back);
  }
  if (!front.empty()) {
    if (!node.front) {
      node.front = new csg_node_t();
    }
    csg_build_node(*node.front, front);
  }
  if (!back.empty()) {
    if (!node.back) {
      node.back = new csg_node_t();
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
  return csg_from_polygons(a.polygons);
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
      std::vector<csg_vertex_t> vertices;
      std::transform(
        info.first.begin(), info.first.end(), std::back_inserter(vertices),
        [&center, &radius, &info](const int i) {
          const auto pos = as::vec3f(
            center.x + radius.x * (2.0f * (i & 1) - 1.0f),
            center.y + radius.y * (2.0f * (i & 2) - 1.0f),
            center.z + radius.z * (2.0f * (i & 4) - 1.0f));
          return csg_vertex_t{.pos = pos, .normal = info.second};
        });
      return csg_polygon_from_vertices(vertices);
    });
  return csg_t{.polygons = polygons};
}

void csg_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height) {
  main_view_ = main_view;
  ortho_view_ = ortho_view;
  screen_dimension_ = {width, height};

  perspective_projection_ = as::perspective_direct3d_lh(
    as::radians(60.0f), float(width) / float(height), 0.01f, 1000.0f);

  asci::Cameras& cameras = camera_system_.cameras_;
  cameras.addCamera(&orbit_rotate_camera_);
  cameras.addCamera(&orbit_dolly_camera_);

  target_camera_.offset = as::vec3(0.0f, 0.0f, -10.0f);
  target_camera_.pitch = as::radians(30.0f);
  target_camera_.yaw = as::radians(45.0f);
  camera_ = target_camera_;

  cube_ = csg_cube(as::vec3f::zero(), as::vec3f(0.5f, 0.5f, 0.5f));
}

void csg_scene_t::input(const SDL_Event& current_event) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void csg_scene_t::update(debug_draw_t& debug_draw, const float delta_time) {
}
