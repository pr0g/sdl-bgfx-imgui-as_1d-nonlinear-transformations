#include "rubiks-cube-scene.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>

#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>

void rubiks_cube_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height) {
  main_view_ = main_view;
  ortho_view_ = ortho_view;
  screen_dimension_ = {width, height};

  perspective_projection_ = as::perspective_direct3d_lh(
    as::radians(60.0f), float(width) / float(height), 0.01f, 1000.0f);

  asci::Cameras& cameras = camera_system_.cameras_;
  cameras.addCamera(&first_person_rotate_camera_);
  cameras.addCamera(&first_person_translate_camera_);

  target_camera_.pivot = as::vec3(0.0f, 0.0f, -10.0f);
  camera_ = target_camera_;

  const float padding = 1.0f;
  const float total_padding = padding * 2.0f;
  const float half_padding = total_padding * 0.5f;

  const float scale = 1.0f;
  as::index offset = 1;
  // top left corner
  const as::vec3 starting_position = as::vec3(
    static_cast<float>(-offset), static_cast<float>(offset),
    static_cast<float>(-offset));
  as::index piece_index = 0;
  for (as::index r = 0; r < 3; r++) {
    for (as::index d = 0; d < 3; d++) {
      for (as::index c = 0; c < 3; c++) {
        // skip center block
        if (r == 1 && c == 1 && d == 1) {
          continue;
        }
        const as::vec3 position_offset = as::vec3(
          static_cast<float>(c), static_cast<float>(-r), static_cast<float>(d));
        const as::vec3 padding_offset = position_offset * padding;

        const as::vec3 piece_position =
          starting_position
          + as::vec3(-half_padding, half_padding, -half_padding)
          + position_offset + padding_offset;
        piece_t& piece = rubiks_cube_.pieces_[piece_index++];
        piece.translation_ = piece_position;

        // corner - blue
        // center - red
        // edge - green

        piece.piece_type_ =
          (r == 0 || r == 2) && (d == 0 || d == 2) && (c == 0 || c == 2)
            ? piece_type_e::corner
          : ((r == 0 || r == 2) && c == 1 && d == 1)
              || ((c == 0 || c == 2) && d == 1 && r == 1)
              || ((d == 0 || d == 2) && c == 1 && r == 1)
            ? piece_type_e::center
            : piece_type_e::edge;
      }
    }
  }
}

void rubiks_cube_scene_t::input(const SDL_Event& current_event) {
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void rubiks_cube_scene_t::update(
  debug_draw_t& debug_draw, const float delta_time) {
  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ = asci::smoothCamera(
    camera_, target_camera_, asci::SmoothProps{}, delta_time);

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  for (as::index i = 0; i < rubiks_cube_.pieces_.size(); i++) {
    debug_draw.debug_cubes->addCube(
      as::mat4_from_vec3(rubiks_cube_.pieces_[i].translation_),
      rubiks_cube_.pieces_[i].piece_type_ == piece_type_e::center
        ? dbg::encodeColorAbgr(1.0f, 0.0f, 0.0f, 1.0f)
      : rubiks_cube_.pieces_[i].piece_type_ == piece_type_e::edge
        ? dbg::encodeColorAbgr(0.0f, 1.0f, 0.0f, 1.0f)
        : dbg::encodeColorAbgr(0.0f, 0.0f, 1.0f, 1.0f));
  }

  // debug_draw.debug_quads->addQuad(
  //   as::mat4_from_vec3(as::vec3::zero()),
  //   dbg::encodeColorAbgr(1.0f, 0.0f, 0.0f, 1.0f));
}
