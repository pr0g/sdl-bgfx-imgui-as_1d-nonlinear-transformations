#pragma once

#include <as/as-math-ops.hpp>

struct plane_t {
  as::vec3f normal;
  float w;
};

inline plane_t plane_from_points(
  const as::vec3f& a, const as::vec3f& b, const as::vec3f& c) {
  const auto n = as::vec_normalize(as::vec3_cross(b - a, c - a));
  return {n, as::vec_dot(n, a)};
}

inline plane_t flip_plane(const plane_t& plane) {
  return {-plane.normal, -plane.w};
}

inline float intersect_plane(
  const as::vec3& origin, const as::vec3& direction, const plane_t& plane) {
  return -(as::vec_dot(origin, plane.normal) + plane.w)
       / as::vec_dot(direction, plane.normal);
}
