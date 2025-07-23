#pragma once

// This implementation of constructive solid geometry is derived from the
// excellent csg.js project (https://github.com/evanw/csg.js), created by Evan
// Wallace. This project ports the JavaScript implementation to C++20, and uses
// bgfx for rendering. The style and algorithms have been kept as close as
// possible to the original. All credit for these go to Evan Wallace.

// Original comments are preserved with a @evanw identifier

#include "plane.h"

#include <as/as-math-ops.hpp>

#include <vector>

// @evanw
// original:
// https://github.com/evanw/csg.js/blob/a8512afbac3cf503195870f7ef11c0a32f36c6d4/csg.js#L1-L44
//
// Constructive Solid Geometry (CSG) is a modelling technique that uses Boolean
// operations like union and intersection to combine 3D solids. This library
// implements CSG operations on meshes elegantly and concisely using BSP trees,
// and is meant to serve as an easily understandable implementation of the
// algorithm. All edge cases involving overlapping coplanar polygons in both
// solids are correctly handled.
//
// Example usage:
//
//     (@thh edit - updated snippet to C++)
//
//     const csg_t cube = csg_cube();
//     const csg_t sphere = csg_sphere();
//     const csg_t polygons = csg_subtract(cube, sphere).polygons;
//
// ## Implementation Details
//
//     (@thh edit - updated function names to C++ equivalents)
//
// All CSG operations are implemented in terms of two functions, `csg_clip_to()`
// and `csg_invert()`, which remove parts of a BSP tree inside another BSP tree
// and swap solid and empty space, respectively. To find the union of `a` and
// `b`, we want to remove everything in `a` inside `b` and everything in `b`
// inside `a`, then combine polygons from `a` and `b` into one solid:
//
//    (@thh edit - updated snippet to C++)
//
//     csg_clip_to(a, b);
//     csg_clip_to(b, a);
//     csg_build_node(a, csg_all_polygons(b));
//
// The only tricky part is handling overlapping coplanar polygons in both trees.
// The code above keeps both copies, but we need to keep them in one tree and
// remove them in the other tree. To remove them from `b` we can clip the
// inverse of `b` against `a`. The code for union now looks like this:
//
//    (@thh edit - updated snippet to C++)
//
//     csg_clip_to(a, b);
//     csg_clip_to(b, a);
//     csg_invert(b)
//     csg_clip_to(b, a);
//     csg_invert(b)
//     csg_build_node(a, csg_all_polygons(b));
//
// Subtraction and intersection naturally follow from set operations. If
// union is `A | B`, subtraction is `A - B = ~(~A | B)` and intersection is
// `A & B = ~(~A | ~B)` where `~` is the complement operator.
//
// ## License
//
// Original work Copyright (c) 2011 Evan Wallace (http://madebyevan.com/), under
// the MIT license.
// Modified work Copyright (c) 2025 Tom Hulton-Harrop
// (http://tomhultonharrop.com/), under the MIT license.

// Types

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

// # struct csg_t
// @evanw
// Holds a binary space partition tree representing a 3D solid. Two solids can
// be combined using the `csg_union()`, `csg_subtract()`, and `csg_intersect()`
// methods.

struct csg_t {
  csg_polygons_t polygons;
};

struct csg_node_t {
  std::optional<plane_t> plane;
  std::unique_ptr<csg_node_t> front;
  std::unique_ptr<csg_node_t> back;
  csg_polygons_t polygons;
};

// Type operations

struct csg_vertex_hash_t {
  std::size_t operator()(const csg_vertex_t& vertex) const noexcept;
};

struct csg_vertex_equals_t {
  bool operator()(
    const csg_vertex_t& lhs, const csg_vertex_t& rhs) const noexcept {
    return lhs.pos == rhs.pos && lhs.normal == rhs.normal;
  }
};

// CSG functions

inline csg_vertex_t csg_interpolate(
  const csg_vertex_t& lhs, const csg_vertex_t& rhs, const float t) {
  return {
    as::vec_mix(lhs.pos, rhs.pos, t), as::vec_mix(lhs.normal, rhs.normal, t)};
}

csg_polygon_t csg_flip_polygon(const csg_polygon_t& polygon);
csg_polygon_t csg_polygon_from_vertices(const csg_vertices_t& vertices);

void csg_split_polygon_by_plane(
  const csg_polygon_t& polygon_t, const plane_t& plane,
  csg_polygons_t& coplanar_front, csg_polygons_t& coplanar_back,
  csg_polygons_t& front, csg_polygons_t& back);

csg_polygons_t csg_clip_polygons(
  const csg_node_t& node, const csg_polygons_t& polygons);

csg_polygons_t csg_all_polygons(const csg_node_t& node);

void csg_build_node(csg_node_t& node, const csg_polygons_t& polygons);

void csg_invert(csg_node_t& node);
void csg_clip_to(csg_node_t& node_lhs, csg_node_t& node_rhs);

// csg_union
// @evanw
// Return a new CSG solid representing space in either this solid or in the
// solid `csg`. Neither this solid nor the solid `csg` are modified.
//
//     A.union(B)
//
//     +-------+            +-------+
//     |       |            |       |
//     |   A   |            |       |
//     |    +--+----+   =   |       +----+
//     +----+--+    |       +----+       |
//          |   B   |            |       |
//          |       |            |       |
//          +-------+            +-------+
//
csg_t csg_union(const csg_t& lhs, const csg_t& rhs);

// csg_subtract
// @evanw
// Return a new CSG solid representing space in this solid but not in the
// solid `csg`. Neither this solid nor the solid `csg` are modified.
//
//     A.subtract(B)
//
//     +-------+            +-------+
//     |       |            |       |
//     |   A   |            |       |
//     |    +--+----+   =   |    +--+
//     +----+--+    |       +----+
//          |   B   |
//          |       |
//          +-------+
//
csg_t csg_subtract(const csg_t& lhs, const csg_t& rhs);

// csg_intersect
// @evanw
// Return a new CSG solid representing space both this solid and in the
// solid `csg`. Neither this solid nor the solid `csg` are modified.
//
//     A.intersect(B)
//
//     +-------+
//     |       |
//     |   A   |
//     |    +--+----+   =   +--+
//     +----+--+    |       +--+
//          |   B   |
//          |       |
//          +-------+
//
csg_t csg_intersect(const csg_t& lhs, const csg_t& rhs);

// csg_inverse
// @evanw
// Return a new CSG solid with solid and empty space switched. This solid is
// not modified.
// todo - implement csg_inverse

// Primitive shapes

csg_t csg_cube(
  const as::vec3f& position = as::vec3f::zero(),
  const as::vec3f& radius = as::vec3::one());
csg_t csg_sphere(
  const as::vec3f& position = as::vec3f::zero(), float radius = 1.0f);
csg_t csg_cylinder(
  const as::vec3f& start = -as::vec3f::axis_y(),
  const as::vec3f& end = as::vec3f::axis_y(), float radius = 1.0f);
