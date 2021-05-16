#pragma once

union SDL_Event;

namespace bgfx
{

struct ProgramHandle;

}

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
  virtual void setup(uint16_t width, uint16_t height) = 0;
  virtual void input(const SDL_Event& current_event) = 0;
  virtual void update(debug_draw_t& debug_draw) = 0;
  virtual void teardown() = 0;

  virtual uint16_t main_view() const = 0;
  virtual uint16_t ortho_view() const = 0;
  virtual bool quit() const = 0;
  virtual bgfx::ProgramHandle simple_handle() const = 0;
  virtual bgfx::ProgramHandle instance_handle() const = 0;

  virtual ~scene_t() = default;
};
