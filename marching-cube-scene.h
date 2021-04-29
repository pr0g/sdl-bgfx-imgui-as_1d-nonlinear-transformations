#pragma once

#include "bgfx/bgfx.h"

struct marching_cube_scene_t
{
  const bgfx::ViewId main_view = 0;
  const bgfx::ViewId gizmo_view = 1;
};

void setup(marching_cube_scene_t& scene, uint16_t width, uint16_t height);
void update(marching_cube_scene_t& scene);
void teardown(marching_cube_scene_t& scene);
