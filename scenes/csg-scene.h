#pragma once

#include "csg/csg.h"
#include "render-thing.h"
#include "scene.h"

#include <as-camera-input/as-camera-input.hpp>
#include <bgfx/bgfx.h>
#include <thh-handle-vector/handle-vector.hpp>

#include <unordered_map>
#include <variant>

enum class operation_e { csg_union, csg_intersection, csg_difference };
enum class shape_e { cube, sphere, cylinder };

struct csg_shape_t {
  shape_e shape = shape_e::cube; // default
  csg_t csg;
  char name[64];
  as::mat4f transform;
  thh::handle_t render_thing_handle;
};

struct csg_operation_t {
  operation_e operation_type = operation_e::csg_union; // default
  std::function<csg_t(const csg_t&, const csg_t&)> operation;
  char name[64];
  char lhs_name[64];
  char rhs_name[64];
  thh::handle_t render_thing_handle;
  thh::handle_t lhs_handle;
  thh::handle_t rhs_handle;
};

using csg_kind_t = std::variant<csg_shape_t, csg_operation_t>;
using csg_kinds_t = thh::handle_vector_t<csg_kind_t>;

struct csg_scene_t : public scene_t {
  void setup(
    bgfx::ViewId main_view, bgfx::ViewId ortho_view, uint16_t width,
    uint16_t height) override;
  void input(const SDL_Event& current_event) override;
  void update(debug_draw_t& debug_draw, float delta_time) override;
  void teardown() override;

  as::vec2i screen_dimension_;
  as::mat4 perspective_projection_;

  asc::Camera camera_;
  asc::Camera target_camera_;
  asci::CameraSystem camera_system_;
  asci::RotateCameraInput first_person_rotate_camera_{asci::MouseButton::Left};
  asci::TranslateCameraInput first_person_translate_camera_{
    asci::lookTranslation, asci::translatePivot};

  bgfx::ViewId main_view_;
  bgfx::ViewId ortho_view_;

  thh::handle_vector_t<render_thing_t> render_things_;

  bgfx::UniformHandle u_light_pos_;
  bgfx::UniformHandle u_camera_pos_;
  as::vec3 light_pos_{0.0f, 8.0f, 8.0f};

  bool wireframe_ = true;
  bool normals_ = false;

  csg_kinds_t root_csg_kinds_;
  csg_kinds_t child_csg_kinds_;
};
