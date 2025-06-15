#pragma once

#include <array>
#include <vector>

#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>
#include <as/as-math-ops.hpp>
#include <bgfx/bgfx.h>

// tagged union - may be better to use variant
// would need to have an array of colors depending on face
// need a way to determine orientation/direction?

enum class side_e {
  front,
  back,
  up,
  down,
  left,
  right,
  middle_lr,
  equator_ud,
  standing_fb
};
std::array<as::index, 9> side(side_e side);

struct sticker_t {
  as::quat rotation_;
  uint32_t color_;
};

enum class piece_type_e { corner, edge, center, core };

struct piece_t {
  as::vec3 translation_;
  as::quat rotation_ = as::quat::identity();
  piece_type_e piece_type_;
  as::index index_ = 0;
  // size is 1, 2 or 3 depending on edge, corner or center
  std::vector<sticker_t> stickers_;
};

// 6 centers
// 12 edges
// 8 corners
// 1 core (hidden)
struct rubiks_cube_t {
  std::array<piece_t, 27> pieces_;
  std::array<as::index, 27> slots_;
};

struct rubiks_cube_scene_t : public scene_t {
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override {}

  as::vec2i screen_dimension_;
  as::mat4 perspective_projection_;

  asc::Camera camera_;
  asc::Camera target_camera_;
  asci::CameraSystem camera_system_;
  asci::RotateCameraInput orbit_rotate_camera_{asci::MouseButton::Left};
  asci::PivotDollyScrollCameraInput orbit_dolly_camera_{};

  bgfx::ViewId main_view_;
  bgfx::ViewId ortho_view_;

  rubiks_cube_t rubiks_cube_;
};
