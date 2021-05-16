#pragma once

#include "fps.h"
#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>
#include <as/as-math-ops.hpp>
#include <bgfx/bgfx.h>
#include <thh-bgfx-debug/debug-shader.hpp>

#include <unordered_map>

namespace mc
{

struct Point;
struct CellValues;
struct CellPositions;

} // namespace mc

template<class T>
inline void hashCombine(std::size_t& seed, const T& v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{

template<>
struct hash<as::vec3>
{
  std::size_t operator()(const as::vec3& vec) const
  {
    size_t seed = 0;
    hashCombine(seed, vec.x);
    hashCombine(seed, vec.y);
    hashCombine(seed, vec.z);
    return seed;
  }
};

} // namespace std

struct Vec3EqualFn
{
  bool operator()(const as::vec3& lhs, const as::vec3& rhs) const
  {
    return as::vec_near(lhs, rhs);
  }
};

enum class Scene
{
  Noise,
  Sphere
};

struct marching_cube_scene_t : public scene_t
{
  void setup(uint16_t width, uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw) override;
  void teardown() override;

  uint16_t main_view() const override { return main_view_; }
  uint16_t ortho_view() const override { return ortho_view_; }
  bool quit() const override { return quit_; }

  bgfx::ProgramHandle simple_handle() const override
  {
    return simple_program.handle();
  };
  bgfx::ProgramHandle instance_handle() const override
  {
    return instance_program.handle();
  };

  bool quit_ = false;

  as::vec2i screen_dimension{};
  const bgfx::ViewId main_view_ = 0;
  const bgfx::ViewId gizmo_view_ = 1;
  const bgfx::ViewId ortho_view_ = 2;

  as::mat4 perspective_projection;

  dbg::EmbeddedShaderProgram simple_program;
  dbg::EmbeddedShaderProgram instance_program;

  bgfx::VertexLayout pos_col_vert_layout;
  bgfx::VertexLayout pos_norm_vert_layout;
  bgfx::VertexBufferHandle cube_col_vbh;
  bgfx::IndexBufferHandle cube_col_ibh;
  bgfx::ProgramHandle program_norm;
  bgfx::ProgramHandle program_col;
  bgfx::UniformHandle u_light_dir;
  bgfx::UniformHandle u_camera_pos;
  as::vec3 light_dir{0.0f, 1.0f, -1.0f};

  asc::Camera camera;
  asc::Camera target_camera;
  asci::RotateCameraInput first_person_rotate_camera{asci::MouseButton::Right};
  asci::PanCameraInput first_person_pan_camera{asci::lookPan};
  asci::TranslateCameraInput first_person_translate_camera{
    asci::lookTranslation};
  asci::ScrollTranslationCameraInput first_person_wheel_camera;
  asci::Cameras cameras;
  asci::CameraSystem camera_system;

  int64_t prev;

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
