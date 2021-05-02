#pragma once

namespace dbg
{
  class DebugCircles;
  class DebugSpheres;
  class DebugLines;
  class DebugCubes;
  class DebugQuads;
}

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
};
