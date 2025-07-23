#pragma once

#include "fps.h"
#include "hash-combine.h"
#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>
#include <as/as-math-ops.hpp>
#include <bgfx/bgfx.h>
#include <thh-bgfx-debug/debug-shader.hpp>

#include <unordered_map>

namespace mc {
struct Point;
struct CellValues;
struct CellPositions;
} // namespace mc

namespace std {

template<>
struct hash<as::vec3> {
  std::size_t operator()(const as::vec3& vec) const {
    size_t seed = 0;
    hash_combine(seed, vec.x);
    hash_combine(seed, vec.y);
    hash_combine(seed, vec.z);
    return seed;
  }
};

} // namespace std

struct Vec3EqualFn {
  bool operator()(const as::vec3& lhs, const as::vec3& rhs) const {
    return as::vec_near(lhs, rhs);
  }
};

enum class Scene { Noise, Sphere };

struct marching_cube_scene_t : public scene_t {
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override;

  as::vec2i screen_dimension{};
  bgfx::ViewId main_view_;
  const bgfx::ViewId gizmo_view_ = 2;

  as::mat4 perspective_projection;

  bgfx::VertexLayout pos_col_vert_layout;
  bgfx::VertexLayout pos_norm_vert_layout;
  bgfx::VertexBufferHandle cube_col_vbh;
  bgfx::IndexBufferHandle cube_col_ibh;
  bgfx::ProgramHandle program_norm;
  bgfx::ProgramHandle program_col;
  bgfx::UniformHandle u_light_dir;
  bgfx::UniformHandle u_camera_pos;
  as::vec3 light_dir{0.0f, 1.0f, -1.0f};

  // cameras default to origin looking down positive z
  asc::Camera camera;
  asc::Camera target_camera;

  asci::RotateCameraInput first_person_rotate_camera{asci::MouseButton::Right};
  asci::PanCameraInput first_person_pan_camera{
    asci::MouseButton::Middle, asci::lookPan, asci::translatePivot};
  asci::TranslateCameraInput first_person_translate_camera{
    asci::lookTranslation, asci::translatePivot};
  asci::ScrollTranslationCameraInput first_person_wheel_camera;
  asci::CameraSystem camera_system;

  const int dimension = 25;
  mc::Point*** points;
  mc::CellValues*** cell_values;
  mc::CellPositions*** cell_positions;

  std::vector<as::vec3> filtered_verts;
  std::vector<as::vec3> filtered_norms;
  std::unordered_map<as::vec3, as::index, std::hash<as::vec3>, Vec3EqualFn>
    unique_verts;

  Scene scene = Scene::Sphere;
  int* scene_alias = nullptr;

  fps::Fps fps;
};
