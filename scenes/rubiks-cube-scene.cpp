#include "rubiks-cube-scene.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>

#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>

const float g_scale = 1.0f;

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
        // piece.rotation_ = ...;

        // rotation test
        // if (c == 0) {
        //   piece.rotation_ = as::quat_rotation_x(as::radians(45.0f));
        // }

        // debug
        // corner - blue
        // center - red
        // edge - green

        const auto piece_type =
          (r == 0 || r == 2) && (d == 0 || d == 2) && (c == 0 || c == 2)
            ? piece_type_e::corner
          : ((r == 0 || r == 2) && c == 1 && d == 1)
              || ((c == 0 || c == 2) && d == 1 && r == 1)
              || ((d == 0 || d == 2) && c == 1 && r == 1)
            ? piece_type_e::center
            : piece_type_e::edge;
        piece.piece_type_ = piece_type;

        if (piece_type == piece_type_e::center) {
          // front face - red
          if (d == 0 && c == 1 && r == 1) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255),
                .rotation_ = as::quat::identity()});
          }
          // back face - orange
          if (d == 2 && c == 1 && r == 1) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00A5FF,
                .rotation_ = as::quat_rotation_y(as::k_pi)});
          }
          // left face - green
          if (d == 1 && c == 0 && r == 1) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00FF00,
                .rotation_ = as::quat_rotation_y(as::k_half_pi)});
          }
          // right face - blue
          if (d == 1 && c == 2 && r == 1) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFF0000,
                .rotation_ = as::quat_rotation_y(-as::k_half_pi)});
          }
          // top face - white
          if (d == 1 && c == 1 && r == 0) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFFFFFF,
                .rotation_ = as::quat_rotation_x(as::k_half_pi)});
          }
          // bottom face - yellow
          if (d == 1 && c == 1 && r == 2) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00FFFF,
                .rotation_ = as::quat_rotation_x(-as::k_half_pi)});
          }
        } else if (piece_type == piece_type_e::edge) {
          // top white edge faces
          if (
            (d == 0 && r == 0 && c == 1) || (d == 2 && r == 0 && c == 1)
            || (d == 1 && r == 0 && c == 0) || (d == 1 && r == 0 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFFFFFF,
                .rotation_ = as::quat_rotation_x(as::k_half_pi)});
          }
          // bottom yellow edge faces
          if (
            (d == 0 && r == 2 && c == 1) || (d == 2 && r == 2 && c == 1)
            || (d == 1 && r == 2 && c == 0) || (d == 1 && r == 2 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00FFFF,
                .rotation_ = as::quat_rotation_x(-as::k_half_pi)});
          }
          // left green edge faces
          if (
            (d == 1 && c == 0 && r == 0) || (d == 1 && c == 0 && r == 2)
            || (d == 0 && c == 0 && r == 1) || (d == 2 && c == 0 && r == 1)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00FF00,
                .rotation_ = as::quat_rotation_y(as::k_half_pi)});
          }
          // right blue edge faces
          if (
            (d == 1 && c == 2 && r == 0) || (d == 1 && c == 2 && r == 2)
            || (d == 0 && c == 2 && r == 1) || (d == 2 && c == 2 && r == 1)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFF0000,
                .rotation_ = as::quat_rotation_y(-as::k_half_pi)});
          }
          // front red edge faces
          if (
            (d == 0 && r == 0 && c == 1) || (d == 0 && r == 2 && c == 1)
            || (d == 0 && r == 1 && c == 0) || (d == 0 && r == 1 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255),
                .rotation_ = as::quat::identity()});
          }
          // back orange edge faces
          if (
            (d == 2 && r == 0 && c == 1) || (d == 2 && r == 2 && c == 1)
            || (d == 2 && r == 1 && c == 0) || (d == 2 && r == 1 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00A5FF,
                .rotation_ = as::quat_rotation_y(as::k_pi)});
          }
        } else if (piece_type == piece_type_e::corner) {
          // front red corner pieces
          if (
            (d == 0 && r == 0 && c == 0) || (d == 0 && r == 0 && c == 2)
            || (d == 0 && r == 2 && c == 0) || (d == 0 && r == 2 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = dbg::encodeColorAbgr((uint8_t)255, 0, 0, 255),
                .rotation_ = as::quat::identity()});
          }
          // back orange corner pieces
          if (
            (d == 2 && r == 0 && c == 0) || (d == 2 && r == 0 && c == 2)
            || (d == 2 && r == 2 && c == 0) || (d == 2 && r == 2 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00A5FF,
                .rotation_ = as::quat_rotation_y(as::k_pi)});
          }
          // top white corner pieces
          if (
            (d == 0 && r == 0 && c == 0) || (d == 0 && r == 0 && c == 2)
            || (d == 2 && r == 0 && c == 0) || (d == 2 && r == 0 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFFFFFF,
                .rotation_ = as::quat_rotation_x(as::k_half_pi)});
          }
          // bottom yellow corner pieces
          if (
            (d == 0 && r == 2 && c == 0) || (d == 0 && r == 2 && c == 2)
            || (d == 2 && r == 2 && c == 0) || (d == 2 && r == 2 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00FFFF,
                .rotation_ = as::quat_rotation_x(-as::k_half_pi)});
          }
          // left green corner pieces
          if (
            (d == 0 && c == 0 && r == 0) || (d == 0 && c == 0 && r == 2)
            || (d == 2 && c == 0 && r == 0) || (d == 2 && c == 0 && r == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00FF00,
                .rotation_ = as::quat_rotation_y(as::k_half_pi)});
          }
          // right blue edge faces
          if (
            (d == 0 && c == 2 && r == 0) || (d == 0 && c == 2 && r == 2)
            || (d == 2 && c == 2 && r == 0) || (d == 2 && c == 2 && r == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFF0000,
                .rotation_ = as::quat_rotation_y(-as::k_half_pi)});
          }
        }
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
    const uint32_t color =
      rubiks_cube_.pieces_[i].piece_type_ == piece_type_e::center
        ? dbg::encodeColorAbgr(1.0f, 0.0f, 0.0f, 1.0f)
      : rubiks_cube_.pieces_[i].piece_type_ == piece_type_e::edge
        ? dbg::encodeColorAbgr(0.0f, 1.0f, 0.0f, 1.0f)
        : dbg::encodeColorAbgr(0.0f, 0.0f, 1.0f, 1.0f);

    debug_draw.debug_cubes->addCube(
      as::mat4_from_mat3(as::mat3_from_quat(rubiks_cube_.pieces_[i].rotation_))
        * as::mat4_from_vec3(rubiks_cube_.pieces_[i].translation_),
      color);

    const as::vec3 sticker_offset =
      as::vec3(-g_scale * 0.5f, -g_scale * 0.5f, -g_scale * 0.5f);

    const as::mat4 sticker_rotation = as::mat4::identity();
    // as::mat4_from_mat3(as::mat3_rotation_y(as::radians(90.0f)));

    // debug_draw.debug_quads->addQuad(
    //   as::mat4_from_mat3(as::mat3_from_quat(rubiks_cube_.pieces_[i].rotation_))
    //     * as::mat4_from_vec3(rubiks_cube_.pieces_[i].translation_)
    //     * sticker_rotation * as::mat4_from_vec3(sticker_offset),
    //   color);

    for (as::index s = 0; s < rubiks_cube_.pieces_[i].stickers_.size(); s++) {
      debug_draw.debug_quads->addQuad(
        as::mat4_from_mat3(
          as::mat3_from_quat(rubiks_cube_.pieces_[i].rotation_))
          * as::mat4_from_vec3(rubiks_cube_.pieces_[i].translation_)
          * as::mat4_from_mat3(
            as::mat3_from_quat(rubiks_cube_.pieces_[i].stickers_[s].rotation_))
          * as::mat4_from_vec3(sticker_offset),
        rubiks_cube_.pieces_[i].stickers_[s].color_);
    }
  }

  // debug_draw.debug_quads->addQuad(
  //   as::mat4_from_vec3(as::vec3::zero()),
  //   dbg::encodeColorAbgr(1.0f, 0.0f, 0.0f, 1.0f));
}
