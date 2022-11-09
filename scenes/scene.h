#pragma once

#include <cstdint>

union SDL_Event;

namespace bgfx
{

struct ProgramHandle;
typedef uint16_t ViewId;

} // namespace bgfx

namespace dbg
{

class DebugCircles;
class DebugSpheres;
class DebugLines;
class DebugCubes;
class DebugQuads;

} // namespace dbg

struct debug_draw_t
{
  dbg::DebugCircles* debug_circles = nullptr;
  dbg::DebugSpheres* debug_spheres = nullptr;
  dbg::DebugLines* debug_lines = nullptr;
  dbg::DebugLines* debug_lines_screen = nullptr;
  dbg::DebugCubes* debug_cubes = nullptr;
  dbg::DebugQuads* debug_quads = nullptr;
};

struct scene_t
{
  virtual ~scene_t() = default;

  virtual void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) = 0;
  virtual void input(const SDL_Event& current_event) = 0;
  virtual void update(debug_draw_t& debug_draw, float delta_time) = 0;
  virtual void teardown() = 0;
};
