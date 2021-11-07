#pragma once

#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Utils/vector_traits.hh>
#include <as/as-math-ops.hpp>

#include <deque>
#include <queue>
#include <variant>
#include <vector>

namespace OpenMesh
{

template<>
struct vector_traits<as::vec3>
{
  //! Type of the vector class
  using vector_type = as::vec3;

  //! Type of the scalar value
  using value_type = float;

  //! size/dimension of the vector
  static const size_t size_ = as::vec3::size();

  //! size/dimension of the vector
  static size_t size() { return as::vec3::size(); }
};

template<>
struct vector_traits<as::vec2>
{
  //! Type of the vector class
  using vector_type = as::vec2;

  //! Type of the scalar value
  using value_type = float;

  static const size_t size_ = as::vec2::size();

  //! size/dimension of the vector
  static size_t size() { return as::vec2::size(); }
};

template<>
inline void vector_cast(
  const as::vec3& src, OpenMesh::Vec3f& dst, GenProg::Int2Type<3> /*unused*/)
{
  dst[0] = static_cast<vector_traits<Vec3f>::value_type>(src.x);
  dst[1] = static_cast<vector_traits<Vec3f>::value_type>(src.y);
  dst[2] = static_cast<vector_traits<Vec3f>::value_type>(src.z);
}

template<>
inline void vector_cast(
  const as::vec2& src, OpenMesh::Vec2f& dst, GenProg::Int2Type<2> /*unused*/)
{
  dst[0] = static_cast<vector_traits<Vec2f>::value_type>(src.x);
  dst[1] = static_cast<vector_traits<Vec2f>::value_type>(src.y);
}

template<>
inline void vector_cast(
  const OpenMesh::Vec3f& src, as::vec3& dst, GenProg::Int2Type<3> /*unused*/)
{
  dst.x = static_cast<vector_traits<Vec3f>::value_type>(src[0]);
  dst.y = static_cast<vector_traits<Vec3f>::value_type>(src[1]);
  dst.z = static_cast<vector_traits<Vec3f>::value_type>(src[2]);
}

template<>
inline void vector_cast(
  const OpenMesh::Vec2f& src, as::vec2& dst, GenProg::Int2Type<2> /*unused*/)
{
  dst.x = static_cast<vector_traits<Vec2f>::value_type>(src[0]);
  dst.y = static_cast<vector_traits<Vec2f>::value_type>(src[1]);
}

template<>
inline void vector_cast(
  const as::vec3& src, OpenMesh::Vec3d& dst, GenProg::Int2Type<3> /*unused*/)
{
  dst[0] = static_cast<vector_traits<Vec3d>::value_type>(src.x);
  dst[1] = static_cast<vector_traits<Vec3d>::value_type>(src.y);
  dst[2] = static_cast<vector_traits<Vec3d>::value_type>(src.z);
}

template<>
inline void vector_cast(
  const as::vec2& src, OpenMesh::Vec2d& dst, GenProg::Int2Type<2> /*unused*/)
{
  dst[0] = static_cast<vector_traits<Vec2d>::value_type>(src.x);
  dst[1] = static_cast<vector_traits<Vec2d>::value_type>(src.y);
}

template<>
inline void vector_cast(
  const OpenMesh::Vec3d& src, as::vec3& dst, GenProg::Int2Type<3> /*unused*/)
{
  dst.x = static_cast<float>(src[0]);
  dst.y = static_cast<float>(src[1]);
  dst.z = static_cast<float>(src[2]);
}

template<>
inline void vector_cast(
  const OpenMesh::Vec2d& src, as::vec2& dst, GenProg::Int2Type<2> /*unused*/)
{
  dst.x = static_cast<float>(src[0]);
  dst.y = static_cast<float>(src[1]);
}

} // namespace OpenMesh

namespace dbg
{

class DebugLines;
class DebugCircles;

} // namespace dbg

namespace vor
{

struct Voronoi : public OpenMesh::DefaultTraits
{
  using Point = as::vec3;
  using Normal = as::vec3;
};

using Mesh = OpenMesh::TriMesh_ArrayKernelT<Voronoi>;

struct site_t
{
  as::vec2 position_;
};

inline bool operator<(const site_t& lhs, const site_t& rhs)
{
  return lhs.position_.y < rhs.position_.y;
}

struct aabb_t
{
  as::vec2 min_;
  as::vec2 max_;
};

struct site_event_t
{
  site_t site_;
};

struct circle_event_t
{
};

using event_t = std::variant<site_event_t, circle_event_t>;

struct voronoi_t
{
  std::vector<site_t> sites_;

  void calculate();

  void display(dbg::DebugLines* lines, dbg::DebugCircles* circles);

  aabb_t bounds_;
  float sweep_line_;

  std::deque<site_t> site_triple_;
  std::vector<as::vec3> circles_;
  std::vector<as::vec2> beachline_; // sorted

  Mesh mesh_;
};

void display_bounds(const aabb_t& bounds, dbg::DebugLines* lines);

} // namespace vor
