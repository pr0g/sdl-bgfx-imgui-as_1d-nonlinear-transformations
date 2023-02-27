#include "arcball-scene.h"

#include <SDL.h>
#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <imgui.h>
#include <thh-bgfx-debug/debug-circle.hpp>
#include <thh-bgfx-debug/debug-line.hpp>

float aspect(const as::vec2i& screen_dimension)
{
  return (float)screen_dimension.x / (float)screen_dimension.y;
}

void arcball_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  pos_norm_vert_layout_.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
    .end();

  main_view_ = main_view;
  ortho_view_ = ortho_view;
  screen_dimension_ = {width, height};

  perspective_projection_ = as::perspective_direct3d_lh(
    as::radians(90.0f), float(width) / float(height), 0.01f, 1000.0f);

  ship_vertices_ = {
    {as::vec3{3.0f, 0.0f, 0.0f}, as::vec3{0.0f, 0.0f, 1.0f}}, // 0
    {as::vec3{-1.0f, 1.0f, 0.0f}, as::vec3{0.0f, 0.0f, 1.0f}}, // 1
    {as::vec3{-1.0f, -1.0f, 0.0f}, as::vec3{0.0f, 0.0f, 1.0f}}, // 2

    {as::vec3{1.0f, 0.0f, 0.0f},
     as::vec3{0.081528f, -0.978347f, 0.190234f}}, // 4
    {as::vec3{-0.75f, 0.0f, 0.75f},
     as::vec3{0.081528f, -0.978347f, 0.190234f}}, // 5
    {as::vec3{-0.5f, -0.125, 0.0f},
     as::vec3{0.081528f, -0.978347f, 0.190234f}}, // 6

    {as::vec3{1.f, 0.0f, 0.0f}, as::vec3{0.081528f, 0.978347f, 0.190234f}}, // 4
    {as::vec3{-0.5, 0.125, 0.0f},
     as::vec3{0.081528f, 0.978347f, 0.190234f}}, // 7
    {as::vec3{-0.75, 0.0f, 0.75f},
     as::vec3{0.081528f, 0.978347f, 0.190234f}}, // 5

    {as::vec3{-0.75, 0.0f, 0.75f}, as::vec3{-0.948683f, 0.0f, -0.316227f}}, // 5
    {as::vec3{-0.5, 0.125, 0.0f}, as::vec3{-0.948683f, 0.0f, -0.316227f}}, // 7
    {as::vec3{-0.5, -0.125, 0.0f}, as::vec3{-0.948683f, 0.0f, -0.316227f}}, // 6

    {as::vec3{3.0f, 0.0f, 0.0f},
     as::vec3{0.064282f, -0.257129f, -0.964236f}}, // 0
    {as::vec3{-1.0f, -1.0f, 0.0f},
     as::vec3{0.064282f, -0.257129f, -0.964236f}}, // 2
    {as::vec3{-0.75f, 0.0f, -0.25f},
     as::vec3{0.064282f, -0.257129f, -0.964236f}}, // 3

    {as::vec3{3.0f, 0.0f, 0.0f},
     as::vec3{0.064282f, 0.257129f, -0.964236f}}, // 0
    {as::vec3{-0.75f, 0.0f, -0.25f},
     as::vec3{0.064282f, 0.257129f, -0.964236f}}, // 3
    {as::vec3{-1.0f, 1.0f, 0.0f},
     as::vec3{0.064282f, 0.257129f, -0.964236f}}, // 1

    {as::vec3{-1.0f, 1.0f, 0.0f}, as::vec3{-0.707106f, 0.0f, -0.707106f}}, // 1
    {as::vec3{-0.75f, 0.0f, -0.25f},
     as::vec3{-0.707106f, 0.0f, -0.707106f}}, // 3
    {as::vec3{-1.0f, -1.0f, 0.0f}, as::vec3{-0.707106f, 0.0f, -0.707106f}} // 2
  };

  ship_indices_.resize(ship_vertices_.size());
  std::iota(ship_indices_.begin(), ship_indices_.end(), (uint16_t)0);

  ship_norm_vbh_ = bgfx::createVertexBuffer(
    bgfx::makeRef(
      ship_vertices_.data(), ship_vertices_.size() * sizeof(PosNormalVertex)),
    pos_norm_vert_layout_);
  ship_norm_ibh_ = bgfx::createIndexBuffer(bgfx::makeRef(
    ship_indices_.data(), ship_indices_.size() * sizeof(uint16_t)));

  program_norm_ =
    createShaderProgram(
      "shader/basic-lighting/v_basic.bin", "shader/basic-lighting/f_basic.bin")
      .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));

  if (!isValid(program_norm_)) {
    std::terminate();
  }

  u_light_pos_ = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4, 1);
  u_camera_pos_ =
    bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4, 1);

  asci::Cameras& cameras = camera_system_.cameras_;
  cameras.addCamera(&first_person_rotate_camera_);
  cameras.addCamera(&first_person_translate_camera_);
}

static as::vec2 ndc_from_screen(
  const as::vec2i& screen, const as::vec2i& dimension)
{
  return as::vec2(
    (2.0f * screen.x / dimension.x - 1.0f) * aspect(dimension),
    2.0f * screen.y / dimension.y - 1.0f);
}

void arcball_scene_t::input(const SDL_Event& current_event)
{
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));

  if (current_event.type == SDL_MOUSEMOTION) {
    SDL_MouseMotionEvent* mouse_motion = (SDL_MouseMotionEvent*)&current_event;
    mouse_now_.x = mouse_motion->x;
    v_now_.x = (2.0f * mouse_now_.x / screen_dimension_.x - 1.0f)
             * aspect(screen_dimension_);
    mouse_now_.y = screen_dimension_.y - mouse_motion->y;
    v_now_.y = 2.0f * mouse_now_.y / screen_dimension_.y - 1.0f;
  }

  if (current_event.type == SDL_MOUSEBUTTONDOWN) {
    SDL_MouseButtonEvent* mouse_button = (SDL_MouseButtonEvent*)&current_event;
    if (mouse_button->button == SDL_BUTTON_LEFT) {
      dragging_ = true;
      v_down_ = v_now_;
    }
  }

  if (current_event.type == SDL_MOUSEBUTTONUP) {
    SDL_MouseButtonEvent* mouse_button = (SDL_MouseButtonEvent*)&current_event;
    if (mouse_button->button == SDL_BUTTON_LEFT) {
      dragging_ = false;
      q_down_ = q_now_;
      m_down_ = m_now_;
    }
  }

  if (current_event.type == SDL_KEYDOWN) {
    SDL_KeyboardEvent* keyboard_event = (SDL_KeyboardEvent*)&current_event;
    if (keyboard_event->keysym.sym == SDLK_LCTRL) {
      constraint_pressed_ = true;
    }
  }
  if (current_event.type == SDL_KEYUP) {
    SDL_KeyboardEvent* keyboard_event = (SDL_KeyboardEvent*)&current_event;
    if (keyboard_event->keysym.sym == SDLK_LCTRL) {
      constraint_pressed_ = false;
    }
  }
}

static as::vec3 mouse_on_sphere(
  const as::vec2& mouse, const as::vec2& center, const float radius)
{
  auto ball_mouse = as::vec3((mouse - center) / radius, 0.0f);
  auto mag = as::vec_dot(ball_mouse, ball_mouse);
  if (mag > 1.0f) {
    float scale = 1.0f / std::sqrt(mag);
    ball_mouse = as::vec3(as::vec2_from_vec3(ball_mouse * scale), 0.0f);
  } else {
    // sign flipped for lh (positive for rh)
    ball_mouse.z = -std::sqrt(1.0f - mag);
  }
  return ball_mouse;
}

// halve arc between unit vectors v0 and v1
as::vec3 bisect(const as::vec3& v0, const as::vec3& v1)
{
  const as::vec3 v = v0 + v1;
  if (const float length = as::vec_length_sq(v); length < 1.0e-5f) {
    return as::vec3(0.0f, 0.0f, 1.0f);
  } else {
    return v * 1.0f / std::sqrt(length);
  }
}

static void draw_arc(
  dbg::DebugLines& debug_lines_screen, const as::vec3& from, const as::vec3& to,
  const as::vec2& position, const uint32_t color)
{
  const int segment_count = 16;
  as::vec3 pts[segment_count + 1];
  pts[0] = from;
  pts[1] = pts[segment_count] = to;
  for (int i = 0; i < 4; i++) {
    pts[1] = bisect(pts[0], pts[1]);
  }
  const float dot = 2.0f * as::vec_dot(pts[0], pts[1]);
  for (int i = 2; i < segment_count; i++) {
    pts[i] = (pts[i - 1] * dot) - pts[i - 2];
  }
  const auto y_flip = as::vec3(1.0f, -1.0f, 1.0f);
  for (int i = 0; i < segment_count; i++) {
    debug_lines_screen.addLine(
      pts[i] * y_flip + as::vec3_from_vec2(position),
      pts[i + 1] * y_flip + as::vec3_from_vec2(position), color);
  }
}

static void draw_half_arc(
  dbg::DebugLines& debug_lines_screen, const as::vec3& n,
  const as::vec2& position, const uint32_t color)
{
  as::vec3 p, m;
  p.z = 0.0f;
  if (n.z != 1.0f) {
    p.x = -n.y;
    p.y = n.x;
    p = as::vec_normalize(p);
  } else {
    p.x = 0.0f;
    p.y = 1.0f;
  }
  m = as::vec3_cross(p, n);
  // draw front
  draw_arc(debug_lines_screen, p, m, position, color);
  draw_arc(debug_lines_screen, m, -p, position, color);
  // draw back arcs
  draw_arc(debug_lines_screen, -p, -m, position, 0xffbbbb00);
  draw_arc(debug_lines_screen, -m, p, position, 0xffbbbb00);
}

static void draw_constraints(
  dbg::DebugLines& debug_lines_screen, const as::mat3& now,
  const std::optional<as::index> axis_index, const as::vec2& position,
  const bool dragging)
{
  for (as::index i = 0; i < as::mat3::dim(); i++) {
    draw_half_arc(
      debug_lines_screen, as::mat_col(now, i), position,
      [matching_axis = axis_index.has_value() && *axis_index == i] {
        if (matching_axis) {
          return 0xff000000;
        }
        return 0xffffff00;
      }());
  }
}

static as::vec3 constrain_to_axis(
  const as::vec3& unconstrained, const as::vec3& axis)
{
  as::vec3 point_on_plane =
    unconstrained - (axis * as::vec_dot(axis, unconstrained));
  const float norm = as::vec_dot(point_on_plane, point_on_plane);
  if (norm > 0.0f) {
    if (point_on_plane.z > 0.0f) {
      point_on_plane = -point_on_plane;
    }
    return point_on_plane * (1.0f / std::sqrt(norm));
  }
  if (axis.z == 1.0f) {
    return as::vec3(1.0f, 0.0f, 0.0f);
  }
  return as::vec_normalize(as::vec3(-axis.y, axis.x, 0.0f));
}

static as::index nearest_constraint_axis(
  const as::vec3& unconstrained, const as::mat3& axes)
{
  float max = -1.0f;
  as::index nearest = 0;
  for (as::index i = 0; i < as::mat3::dim(); ++i) {
    const as::vec3 point_on_plane =
      constrain_to_axis(unconstrained, as::mat_col(axes, i));
    if (const float dot = as::vec_dot(point_on_plane, unconstrained);
        dot > max) {
      max = dot;
      nearest = i;
    }
  }
  return nearest;
}

void arcball_scene_t::update(debug_draw_t& debug_draw, const float delta_time)
{
  ImGui::Begin("Sphere");
  float position_imgui[2];
  as::vec_to_arr(sphere_position_, position_imgui);
  static bool track = false;
  ImGui::Checkbox("Track position", &track);
  ImGui::SliderFloat2("Position", position_imgui, -1.0f, 1.0f);
  ImGui::SliderFloat("Radius", &sphere_radius_, 0.01f, 1.0f);
  sphere_position_ = as::vec2_from_arr(position_imgui);
  ImGui::End();

  v_from_ = mouse_on_sphere(v_down_, sphere_position_, sphere_radius_);
  v_to_ = mouse_on_sphere(v_now_, sphere_position_, sphere_radius_);
  if (dragging_) {
    if (axis_index_.has_value()) {
      v_from_ = constrain_to_axis(
        v_from_,
        as::mat_col(
          as::mat_mul(m_down_, as::mat3_from_affine(target_camera_.view())),
          *axis_index_));
      v_to_ = constrain_to_axis(
        v_to_,
        as::mat_col(
          as::mat_mul(m_down_, as::mat3_from_affine(target_camera_.view())),
          *axis_index_));
    }
    const as::quat q_drag =
      as::quat(as::vec_dot(v_from_, v_to_), as::vec3_cross(v_from_, v_to_));
    q_now_ = as::quat_from_mat3(as::mat3_from_affine(camera_.transform()))
           * q_drag * as::quat_from_mat3(as::mat3_from_affine(camera_.view()))
           * q_down_;
  } else {
    if (constraint_pressed_) {
      axis_index_ = nearest_constraint_axis(
        v_to_,
        as::mat_mul(m_now_, as::mat3_from_affine(target_camera_.view())));
    } else {
      axis_index_.reset();
    }
  }
  m_now_ = as::mat3_from_quat(q_now_);

  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ = asci::smoothCamera(
    camera_, target_camera_, asci::SmoothProps{}, delta_time);

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  const auto offset = as::mat_mul(
    as::mat4_from_mat3(m_now_), as::mat4_from_vec3(as::vec3::axis_z(8.0f)));

  float model[16];
  as::mat_to_arr(offset, model);

  bgfx::setTransform(model);

  bgfx::setUniform(u_light_pos_, (void*)&light_pos_, 1);
  bgfx::setUniform(u_camera_pos_, (void*)&camera_.pivot, 1);

  bgfx::setVertexBuffer(0, ship_norm_vbh_);
  bgfx::setIndexBuffer(ship_norm_ibh_);
  bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CCW);
  bgfx::submit(main_view_, program_norm_);

  const float ar = aspect(screen_dimension_);
  const as::mat4 orthographic_projection =
    as::ortho_direct3d_lh(-1.0f * ar, 1.0f * ar, 1.0f, -1.0f, -10.0f, 10.0f);

  float proj_o[16];
  as::mat_to_arr(orthographic_projection, proj_o);
  bgfx::setViewTransform(ortho_view_, nullptr, proj_o);

  if (track) {
    const auto pos = as::world_to_screen(
      as::mat4_translation(offset), perspective_projection_, camera_.view(),
      screen_dimension_);
    sphere_position_ = ndc_from_screen(
      as::vec2i(pos.x, screen_dimension_.y - pos.y), screen_dimension_);
  }

  debug_draw.debug_circles_screen->addWireCircle(
    as::mat4_from_mat3_vec3(
      as::mat3_scale(sphere_radius_),
      as::vec3_from_vec2(as::vec2(sphere_position_.x, -sphere_position_.y))),
    0xffffffff);

  debug_draw.debug_lines_screen->setTransform(
    as::mat4_from_mat3(as::mat3_scale(sphere_radius_)));

  if (dragging_) {
    draw_arc(
      *debug_draw.debug_lines_screen, v_from_, v_to_,
      as::vec2(sphere_position_.x, -sphere_position_.y) * 1.0f / sphere_radius_,
      0xffff00ee);
  }

  draw_constraints(
    *debug_draw.debug_lines_screen,
    as::mat_mul(m_now_, as::mat3_from_affine(target_camera_.view())),
    axis_index_,
    as::vec2(sphere_position_.x, -sphere_position_.y) * 1.0f / sphere_radius_,
    dragging_);
}
