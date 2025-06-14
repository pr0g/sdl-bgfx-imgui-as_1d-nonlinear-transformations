#include "rubiks-cube-scene.h"

#include <numeric>

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <easy_iterator.h>
#include <imgui.h>

#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>

namespace ei = easy_iterator;

const float g_scale = 1.0f;

// debug
// corner - blue
// center - red
// edge - green

std::array<as::index, 9> side_slots_indices(const side_e side) {
  switch (side) {
    case side_e::up:
      return std::array<as::index, 9>{0, 1, 2, 3, 4, 5, 6, 7, 8};
    case side_e::equator_ud:
      return std::array<as::index, 9>{9, 10, 11, 12, 13, 14, 15, 16, 17};
    case side_e::down:
      return std::array<as::index, 9>{18, 19, 20, 21, 22, 23, 24, 25, 26};
    case side_e::left:
      return std::array<as::index, 9>{0, 3, 6, 9, 12, 15, 18, 21, 24};
    case side_e::middle_lr:
      return std::array<as::index, 9>{1, 4, 7, 10, 13, 16, 19, 22, 25};
    case side_e::right:
      return std::array<as::index, 9>{2, 5, 8, 11, 14, 17, 20, 23, 26};
    case side_e::front:
      return std::array<as::index, 9>{0, 1, 2, 9, 10, 11, 18, 19, 20};
    case side_e::standing_fb:
      return std::array<as::index, 9>{3, 4, 5, 12, 13, 14, 21, 22, 23};
    case side_e::back:
      return std::array<as::index, 9>{6, 7, 8, 15, 16, 17, 24, 25, 26};
    default:
      break;
  }
}

void rotate(
  rubiks_cube_t& rubiks_cube, const side_e side, const as::quat& rotation) {
  const auto slots = side_slots_indices(side);
  const auto offsets_cw =
    std::array<as::index, 9>{6, 2, -2, 4, 0, -4, 2, -2, -6};

  std::array<as::index, 9> current_indices;
  for (as::index i = 0; i < current_indices.size(); i++) {
    current_indices[i] = rubiks_cube.slots_[slots[i]];
  }

  std::array<as::index, 9> next_indices;
  for (as::index i = 0; i < next_indices.size(); i++) {
    next_indices[i + offsets_cw[i]] = current_indices[i];
  }

  for (as::index i = 0; i < slots.size(); i++) {
    auto& piece = rubiks_cube.pieces_[slots[i]];
    piece.rotation_ = rotation * piece.rotation_;
    rubiks_cube.slots_[slots[i]] = next_indices[i];
  }
}

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

  const as::index offset = 1;
  // up left corner
  const as::vec3 starting_position = as::vec3(
    static_cast<float>(-offset), static_cast<float>(offset),
    static_cast<float>(-offset));
  as::index piece_index = 0;
  for (as::index r = 0; r < 3; r++) {
    for (as::index d = 0; d < 3; d++) {
      for (as::index c = 0; c < 3; c++) {
        const as::vec3 position_offset = as::vec3(
          static_cast<float>(c), static_cast<float>(-r), static_cast<float>(d));
        const as::vec3 padding_offset = position_offset * padding;

        const as::vec3 piece_position =
          starting_position
          + as::vec3(-half_padding, half_padding, -half_padding)
          + position_offset + padding_offset;
        piece_t& piece = rubiks_cube_.pieces_[piece_index];
        piece.translation_ = piece_position;
        piece.rotation_ = as::quat::identity();
        piece.index_ = piece_index++;

        // if (d == 0 && r == 0 && c == 0) {
        //   piece.rotation_ = as::quat_rotation_y(as::k_half_pi);
        //   piece.rotation_ =
        //     as::quat_rotation_x(as::k_half_pi) * piece.rotation_;
        //   piece.rotation_ =
        //     as::quat_rotation_z(as::k_half_pi) * piece.rotation_;
        // }

        const auto piece_type =
          ((r == 1) && (c == 1) && (d == 1))
            ? piece_type_e::core // hidden piece
          : (r == 0 || r == 2) && (d == 0 || d == 2) && (c == 0 || c == 2)
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
                .color_ = 0xFF0000FF, .rotation_ = as::quat::identity()});
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
          // up face - white
          if (d == 1 && c == 1 && r == 0) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFFFFFF,
                .rotation_ = as::quat_rotation_x(as::k_half_pi)});
          }
          // down face - yellow
          if (d == 1 && c == 1 && r == 2) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00FFFF,
                .rotation_ = as::quat_rotation_x(-as::k_half_pi)});
          }
        } else if (piece_type == piece_type_e::edge) {
          // up white edge faces
          if (
            (d == 0 && r == 0 && c == 1) || (d == 2 && r == 0 && c == 1)
            || (d == 1 && r == 0 && c == 0) || (d == 1 && r == 0 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFFFFFF,
                .rotation_ = as::quat_rotation_x(as::k_half_pi)});
          }
          // down yellow edge faces
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
                .color_ = 0xFF0000FF, .rotation_ = as::quat::identity()});
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
                .color_ = 0xFF0000FF, .rotation_ = as::quat::identity()});
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
          // up white corner pieces
          if (
            (d == 0 && r == 0 && c == 0) || (d == 0 && r == 0 && c == 2)
            || (d == 2 && r == 0 && c == 0) || (d == 2 && r == 0 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFFFFFF,
                .rotation_ = as::quat_rotation_x(as::k_half_pi)});
          }
          // down yellow corner pieces
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
            (d == 0 && r == 0 && c == 0) || (d == 0 && r == 2 && c == 0)
            || (d == 2 && r == 0 && c == 0) || (d == 2 && r == 2 && c == 0)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFF00FF00,
                .rotation_ = as::quat_rotation_y(as::k_half_pi)});
          }
          // right blue edge faces
          if (
            (d == 0 && r == 0 && c == 2) || (d == 0 && r == 2 && c == 2)
            || (d == 2 && r == 0 && c == 2) || (d == 2 && r == 2 && c == 2)) {
            piece.stickers_.push_back(
              sticker_t{
                .color_ = 0xFFFF0000,
                .rotation_ = as::quat_rotation_y(-as::k_half_pi)});
          }
        }
      }
    }
  }

  std::iota(
    rubiks_cube_.slots_.begin(), rubiks_cube_.slots_.end(),
    static_cast<as::index>(0));
}

void rubiks_cube_scene_t::input(const SDL_Event& current_event) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void rubiks_cube_scene_t::update(
  debug_draw_t& debug_draw, const float delta_time) {
  ImGui::Begin("Rubik's Cube");
  ImGui::Text(
    " 0 [%2ld],  1 [%2ld],  2 [%2ld]\n"
    " 3 [%2ld],  4 [%2ld],  4 [%2ld]\n"
    " 6 [%2ld],  7 [%2ld],  8 [%2ld]\n"
    " 9 [%2ld], 10 [%2ld], 11 [%2ld]\n"
    "12 [%2ld], 13 [%2ld], 14 [%2ld]\n"
    "15 [%2ld], 16 [%2ld], 17 [%2ld]\n"
    "18 [%2ld], 19 [%2ld], 20 [%2ld]\n"
    "21 [%2ld], 22 [%2ld], 23 [%2ld]\n"
    "24 [%2ld], 25 [%2ld], 26 [%2ld]\n",
    rubiks_cube_.slots_[0], rubiks_cube_.slots_[1], rubiks_cube_.slots_[2],
    rubiks_cube_.slots_[3], rubiks_cube_.slots_[4], rubiks_cube_.slots_[5],
    rubiks_cube_.slots_[6], rubiks_cube_.slots_[7], rubiks_cube_.slots_[8],
    rubiks_cube_.slots_[9], rubiks_cube_.slots_[10], rubiks_cube_.slots_[11],
    rubiks_cube_.slots_[12], rubiks_cube_.slots_[13], rubiks_cube_.slots_[14],
    rubiks_cube_.slots_[15], rubiks_cube_.slots_[16], rubiks_cube_.slots_[17],
    rubiks_cube_.slots_[18], rubiks_cube_.slots_[19], rubiks_cube_.slots_[20],
    rubiks_cube_.slots_[21], rubiks_cube_.slots_[22], rubiks_cube_.slots_[23],
    rubiks_cube_.slots_[24], rubiks_cube_.slots_[25], rubiks_cube_.slots_[26]);

  if (ImGui::Button("Rotate Up")) {
    rotate(rubiks_cube_, side_e::up, as::quat_rotation_y(as::radians(90.0f)));
  }

  if (ImGui::Button("Rotate Equator")) {
    rotate(
      rubiks_cube_, side_e::equator_ud,
      as::quat_rotation_y(as::radians(90.0f)));
  }

  if (ImGui::Button("Rotate Down")) {
    rotate(
      rubiks_cube_, side_e::down, as::quat_rotation_y(-as::radians(90.0f)));
  }

  if (ImGui::Button("Rotate Left")) {
    rotate(
      rubiks_cube_, side_e::left, as::quat_rotation_x(-as::radians(90.0f)));
  }

  if (ImGui::Button("Rotate Middle")) {
    rotate(
      rubiks_cube_, side_e::middle_lr,
      as::quat_rotation_x(-as::radians(90.0f)));
  }

  if (ImGui::Button("Rotate Right")) {
    rotate(
      rubiks_cube_, side_e::right, as::quat_rotation_x(as::radians(90.0f)));
  }

  if (ImGui::Button("Rotate Front")) {
    rotate(
      rubiks_cube_, side_e::front, as::quat_rotation_z(-as::radians(90.0f)));
  }

  if (ImGui::Button("Rotate Standing")) {
    rotate(
      rubiks_cube_, side_e::standing_fb,
      as::quat_rotation_z(as::radians(90.0f)));
  }

  if (ImGui::Button("Rotate Back")) {
    rotate(rubiks_cube_, side_e::back, as::quat_rotation_z(as::radians(90.0f)));
  }

  ImGui::End();

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
      rubiks_cube_.pieces_[i].piece_type_ == piece_type_e::core
        ? dbg::encodeColorAbgr(0.0f, 0.0f, 0.0f, 1.0f)
      : rubiks_cube_.pieces_[i].piece_type_ == piece_type_e::center
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
}
