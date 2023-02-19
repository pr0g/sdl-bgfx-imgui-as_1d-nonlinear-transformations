#include "arcball-scene.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <thh-bgfx-debug/debug-circle.hpp>

#include <SDL.h>

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

void arcball_scene_t::update(debug_draw_t& debug_draw, const float delta_time)
{
  v_from_ = mouse_on_sphere(v_down_, as::vec2::zero(), 0.75f);
  v_to_ = mouse_on_sphere(v_now_, as::vec2::zero(), 0.75f);
  if (dragging_) {
    const auto axis = as::vec3_cross(v_from_, v_to_);
    const auto angle = as::vec_dot(v_from_, v_to_);
    const as::quat q_drag = as::quat(angle, axis);
    q_now_ = q_drag * q_down_;
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

  const float radius = 0.75f;

  auto offset = as::mat4_from_mat3_vec3(
    as::mat3_scale(radius * (1.0f / 3.0f)), as::vec3::axis_z(2.0f));

  offset = as::mat_mul(as::mat4_from_mat3(m_now_), offset);

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
    as::ortho_direct3d_lh(-1.0f * ar, 1.0f * ar, 1.0f, -1.0f, 0.0f, 1.0f);

  float proj_o[16];
  as::mat_to_arr(orthographic_projection, proj_o);

  bgfx::setViewTransform(ortho_view_, nullptr, proj_o);

  debug_draw.debug_circles_screen->addWireCircle(
    as::mat4_from_mat3_vec3(
      as::mat3_scale(radius), as::vec3_from_vec2(as::vec2::zero(), 0.5f)),
    0xffffffff);
}
