#include "globe-scene.h"

#include "bgfx-helpers.h"
#include "noise.h"

#include <SDL.h>
#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <imgui.h>
#include <thh-bgfx-debug/debug-circle.hpp>
#include <thh-bgfx-debug/debug-color.hpp>

void globe_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  pos_col_vert_layout_.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();

  main_view_ = main_view;
  ortho_view_ = ortho_view;

  screen_dimension_ = as::vec2i(width, height);
  perspective_projection_ = as::perspective_direct3d_lh(
    as::radians(60.0f), float(width) / float(height), 0.01f, 1000.0f);

  first_person_rotate_camera_.constrain_pitch_ = [] { return false; };

  camera_.pivot = as::vec3::axis_y(11.0f);
  camera_.offset = as::vec3::axis_z(-10.0f);
  target_camera_ = camera_;

  const int loop_count = 50;
  float current_vertical_angle_rad = as::k_pi / 2.0f;

  int rng = 0;
  int seed = 0;

  float inner_rot = 0.0f;
  const float horizontal_increment = as::k_tau / float(loop_count);
  const float vertical_increment = as::k_pi / float(loop_count);
  for (size_t outer = 0; outer < loop_count; ++outer) {
    const float vertical_position_first = std::sin(current_vertical_angle_rad);
    const float vertical_position_second =
      std::sin(current_vertical_angle_rad + vertical_increment);
    const float horizontal_scale_first = std::cos(current_vertical_angle_rad);
    const float horizontal_scale_second =
      std::cos(current_vertical_angle_rad + vertical_increment);
    for (size_t inner = 0; inner < loop_count; ++inner) {
      sphere_vertices.push_back(
        {as::vec3(
           std::cos(inner_rot) * horizontal_scale_first,
           vertical_position_first,
           std::sin(inner_rot) * horizontal_scale_first),
         dbg::encodeColorAbgr(
           ns::noise1dZeroToOne(rng, seed), ns::noise1dZeroToOne(rng, seed),
           ns::noise1dZeroToOne(rng, seed), 1.0f)});
      sphere_vertices.push_back(
        {as::vec3(
           std::cos(inner_rot) * horizontal_scale_second,
           vertical_position_second,
           std::sin(inner_rot) * horizontal_scale_second),
         0xffffffff});
      inner_rot += horizontal_increment;
      rng++;
    }
    current_vertical_angle_rad += vertical_increment;
  }

  const int twice_loop_count = loop_count * 2;
  for (int loop = 0; loop < loop_count; ++loop) {
    for (int i = 0; i < twice_loop_count; i += 2) {
      sphere_indices.push_back(i + (loop * twice_loop_count));
      sphere_indices.push_back(
        (i + 1) % twice_loop_count + (loop * twice_loop_count));
      sphere_indices.push_back(
        (i + 2) % twice_loop_count + (loop * twice_loop_count));
      sphere_indices.push_back(i + 1 + (loop * twice_loop_count));
      sphere_indices.push_back(
        (i + 3) % twice_loop_count + (loop * twice_loop_count));
      sphere_indices.push_back(
        (i + 2) % twice_loop_count + (loop * twice_loop_count));
    }
  }

  sphere_col_vbh_ = bgfx::createVertexBuffer(
    bgfx::makeRef(
      sphere_vertices.data(), sphere_vertices.size() * sizeof(PosColorVertex)),
    pos_col_vert_layout_);
  sphere_col_ibh_ = bgfx::createIndexBuffer(bgfx::makeRef(
    sphere_indices.data(), sphere_indices.size() * sizeof(int16_t)));

  program_col_ = createShaderProgram(
                   "shader/simple/v_simple.bin", "shader/simple/f_simple.bin")
                   .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));

  asci::Cameras& cameras = camera_system_.cameras_;
  cameras.addCamera(&first_person_rotate_camera_);
  cameras.addCamera(&first_person_dolly_camera_);
}

void globe_scene_t::input(const SDL_Event& current_event)
{
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));

  if (current_event.type == SDL_MOUSEBUTTONDOWN) {
    const auto* mouse_button_event = (SDL_MouseButtonEvent*)&current_event;
    rotating_ = mouse_button_event->button == SDL_BUTTON_LEFT;
  }

  if (current_event.type == SDL_MOUSEBUTTONUP) {
    const auto* mouse_button_event = (SDL_MouseButtonEvent*)&current_event;
    rotating_ = false;
  }

  if (rotating_) {
    if (current_event.type == SDL_MOUSEMOTION) {
      const float scale = 0.001f;
      const auto* mouse_motion_event = (SDL_MouseMotionEvent*)&current_event;
      roll_pitch_.x -= (float)mouse_motion_event->xrel * scale;
      roll_pitch_.y -= (float)mouse_motion_event->yrel * scale;
    }
  }
}

void globe_scene_t::update(debug_draw_t& debug_draw, const float delta_time)
{
  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ = asci::smoothCamera(
    camera_, target_camera_, asci::SmoothProps{}, delta_time);

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  const auto forward = [this] {
    const auto forward = as::mat3_basis_z(camera_.rotation());
    return as::vec_normalize(as::vec3(forward.x, 0.0f, forward.z));
  }();

  auto side = as::mat3_basis_x(camera_.rotation());
  auto pitch = as::mat3_rotation_axis(side, roll_pitch_.y);
  auto roll = as::mat3_rotation_axis(forward, roll_pitch_.x);
  roll_pitch_ = as::vec2::zero();

  model_ = as::mat_mul(as::mat_mul(model_, roll), pitch);

  float model[16];
  as::mat_to_arr(
    as::mat4_from_mat3(as::mat_mul(as::mat3_scale(10.0f), model_)), model);

  bgfx::setTransform(model);

  bgfx::setVertexBuffer(0, sphere_col_vbh_);
  bgfx::setIndexBuffer(sphere_col_ibh_);
  bgfx::setState(BGFX_STATE_DEFAULT);
  bgfx::submit(main_view_, program_col_);
}
