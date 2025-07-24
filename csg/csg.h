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

// csg_vertex_t
// @evanw
// Represents a vertex of a polygon. This class provides `normal` so convenience
// functions like `CSG.sphere()` can return a smooth vertex normal, but `normal`
// is not used anywhere else.
// @thh edit - vertex customization point no longer supported (may be updated in
// future)

struct csg_vertex_t {
  as::vec3f pos;
  as::vec3f normal;
};

using csg_vertices_t = std::vector<csg_vertex_t>;

// csg_polygon_t
// @evanw (@thh edit)
// Represents a convex polygon. The vertices used to initialize a polygon must
// be coplanar and form a convex loop.
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

// csg_node_t
// Holds a node in a BSP tree. A BSP tree is built from a collection of polygons
// by picking a polygon to split along. That polygon (and all other coplanar
// polygons) are added directly to that node and the other polygons are added to
// the front and/or back subtrees. This is not a leafy BSP tree since there is
// no distinction between internal and leaf nodes.
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

// csg_interpolate
// @evanw (@thh edit)
// Creates a new vertex between this `lhs` and `rhs` by linearly
// interpolating all properties using a parameter of `t`.
// @thh edit - subclassing no longer supported
inline csg_vertex_t csg_interpolate(
  const csg_vertex_t& lhs, const csg_vertex_t& rhs, const float t) {
  return {
    as::vec_mix(lhs.pos, rhs.pos, t), as::vec_mix(lhs.normal, rhs.normal, t)};
}

// csg_flip_polygon
// Returns a new polygon with vertices reversed and plane flipped
csg_polygon_t csg_flip_polygon(const csg_polygon_t& polygon);

// csg_polygon_from_vertices
// Returns a new polygon from vertices (vertices must be coplanar, convex and
// have a minumum of 3 points)
csg_polygon_t csg_polygon_from_vertices(const csg_vertices_t& vertices);

// csg_split_polygon_by_plane
// @evanw (@thh edit)
// Split `polygon` by `plane` if needed, then put the polygon or polygon
// fragments in the appropriate lists. Coplanar polygons go into either
// `coplanarFront` or `coplanarBack` depending on their orientation with
// respect to this plane. Polygons in front or behind this plane go into
// either `front` or `back`.
void csg_split_polygon_by_plane(
  const csg_polygon_t& polygon, const plane_t& plane,
  csg_polygons_t& coplanar_front, csg_polygons_t& coplanar_back,
  csg_polygons_t& front, csg_polygons_t& back);

// csg_clip_polygons
// @evanw
// Recursively remove all polygons in `polygons` that are inside this BSP tree.
csg_polygons_t csg_clip_polygons(
  const csg_node_t& node, const csg_polygons_t& polygons);

// csg_all_polygons
// @evanw
// Return a list of all polygons in the BSP tree `node`.
csg_polygons_t csg_all_polygons(const csg_node_t& node);

// csg_build_node
// @evanw
// Build a BSP tree out of `polygons`. When called on an existing tree, the
// new polygons are filtered down to the bottom of the tree and become new
// nodes there. Each set of polygons is partitioned using the first polygon
// (no heuristic is used to pick a good split).
void csg_build_node(csg_node_t& node, const csg_polygons_t& polygons);

// csg_invert
// Convert solid space to empty space and empty space to solid space.
void csg_invert(csg_node_t& node);

// csg_clip_to
// @evanw (@thh edit)
// Remove all polygons in `node_lhs` BSP tree that are inside `node_rhs` BSP
// tree.
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
csg_t csg_inverse(const csg_t& csg);

// Primitive shapes

struct csg_cube_config_t {
  as::vec3f center = as::vec3f::zero();
  as::vec3f radius = as::vec3f::one();
};

// csg_cube
// @evanw
// Construct an axis-aligned solid cuboid. Optional parameters are `center` and
// `radius`, which default to `[0, 0, 0]` and `[1, 1, 1]`.
//
// Example code:
//
// csg_cube(
//   csg_cube_config_t{
//     .center = as::vec3f::zero(), .radius = as::vec3f(0.5f, 1.5f, 0.5f)})
csg_t csg_cube(const csg_cube_config_t& config = csg_cube_config_t{});

struct csg_sphere_config_t {
  as::vec3f position = as::vec3f::zero();
  float radius = 1.0f;
  int slices = 16;
  int stacks = 8;
};

// csg_sphere
// @evanw
// Construct a solid sphere. Optional parameters are `center`, `radius`,
// `slices`, and `stacks`, which default to `[0, 0, 0]`, `1`, `16`, and `8`.
// The `slices` and `stacks` parameters control the tessellation along the
// longitude and latitude directions.
//
// Example code:
//
// csg_sphere(
//   csg_sphere_config_t{
//     .position = as::vec3f(0.5f, 0.0f, 0.5f), .radius = 0.8f});
csg_t csg_sphere(const csg_sphere_config_t& config = csg_sphere_config_t{});

struct csg_cylinder_config_t {
  as::vec3f start = -as::vec3f::axis_y();
  as::vec3f end = as::vec3f::axis_y();
  float radius = 1.0f;
  int slices = 16;
};

// csg_cylinder
// @evanw
// Construct a solid cylinder. Optional parameters are `start`, `end`,
// `radius`, and `slices`, which default to `[0, -1, 0]`, `[0, 1, 0]`, `1`, and
// `16`. The `slices` parameter controls the tessellation.
//
// Example code:
//
// csg_cylinder(
//     csg_cylinder_config_t{
//       .start = -as::vec3f::axis_y(),
//       .end = as::vec3f::axis_y(),
//       .radius = 0.5f})
csg_t csg_cylinder(
  const csg_cylinder_config_t& config = csg_cylinder_config_t{});
