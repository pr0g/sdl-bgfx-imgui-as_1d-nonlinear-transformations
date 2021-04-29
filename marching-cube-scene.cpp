#include "marching-cube-scene.h"

#include "bgfx-helpers.h"
#include "file-ops.h"

#include <as/as-math-ops.hpp>
#include <thh-bgfx-debug/debug-shader.hpp>

struct PosColorVertex
{
  as::vec3 position;
  uint32_t abgr;
};

struct PosNormalVertex
{
  as::vec3 position;
  as::vec3 normal;
};

static const PosColorVertex CubeVerticesCol[] = {
  {as::vec3{-1.0f, 1.0f, 1.0f}, 0xff000000},
  {as::vec3{1.0f, 1.0f, 1.0f}, 0xff0000ff},
  {as::vec3{-1.0f, -1.0f, 1.0f}, 0xff00ff00},
  {as::vec3{1.0f, -1.0f, 1.0f}, 0xff00ffff},
  {as::vec3{-1.0f, 1.0f, -1.0f}, 0xffff0000},
  {as::vec3{1.0f, 1.0f, -1.0f}, 0xffff00ff},
  {as::vec3{-1.0f, -1.0f, -1.0f}, 0xffffff00},
  {as::vec3{1.0f, -1.0f, -1.0f}, 0xffffffff},
};

static const uint16_t CubeTriListCol[] = {
  0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7, 0, 2, 4, 4, 2, 6,
  1, 5, 3, 5, 7, 3, 0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7,
};

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

void setup(
  marching_cube_scene_t& scene, const uint16_t width, const uint16_t height)
{
  // cornflower clear colour
  bgfx::setViewClear(
    scene.main_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
  bgfx::setViewRect(scene.main_view, 0, 0, width, height);

  // size and placement of gizmo on screen
  const float gizmo_offset_percent = 0.85f;
  const float gizmo_size_percent = 0.1f;
  bgfx::setViewClear(scene.gizmo_view, BGFX_CLEAR_DEPTH);
  bgfx::setViewRect(
    scene.gizmo_view, width * gizmo_offset_percent,
    height * gizmo_offset_percent, width * gizmo_size_percent,
    height * gizmo_size_percent);

  dbg::EmbeddedShaderProgram simple_program;
  simple_program.init(dbg::SimpleEmbeddedShaderArgs);

  bgfx::VertexLayout pos_col_vert_layout;
  pos_col_vert_layout.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();

  bgfx::VertexLayout pos_norm_vert_layout;
  pos_norm_vert_layout.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
    .end();

  bgfx::VertexBufferHandle cube_col_vbh = bgfx::createVertexBuffer(
    bgfx::makeRef(CubeVerticesCol, sizeof(CubeVerticesCol)),
    pos_col_vert_layout);
  bgfx::IndexBufferHandle cube_col_ibh = bgfx::createIndexBuffer(
    bgfx::makeRef(CubeTriListCol, sizeof(CubeTriListCol)));

  const bgfx::ProgramHandle program_norm =
    createShaderProgram("shader/next/v_next.bin", "shader/next/f_next.bin")
      .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));

  const bgfx::ProgramHandle program_col =
    createShaderProgram(
      "shader/simple/v_simple.bin", "shader/simple/f_simple.bin")
      .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));

  const bgfx::UniformHandle u_light_dir =
    bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4, 1);
  const bgfx::UniformHandle u_camera_pos =
    bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4, 1);

  as::vec3 light_dir{0.0f, 1.0f, -1.0f};
}

void update(marching_cube_scene_t& scene)
{
}

void teardown(marching_cube_scene_t& scene)
{
}
