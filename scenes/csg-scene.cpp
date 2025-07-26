#include "csg-scene.h"

#include "bgfx-helpers.h"
#include "csg/csg.h"
#include "debug.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <easy_iterator.h>
#include <imgui.h>

#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>

#include <format>
#include <ranges>

namespace ei = easy_iterator;

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};

std::string operation_name(int operation) {
  switch (operation) {
    case 0:
      return "union";
    case 1:
      return "intersection";
    case 2:
      return "difference";
  }
  return "";
}

static csg_t create_csg_union() {
  const auto a =
    csg_cube({.min = {-1.25f, -1.25f, -1.25f}, .max = {0.75f, 0.75f, 0.75f}});
  const auto b = csg_sphere({.center = {0.25f, 0.25f, 0.25f}, .radius = 1.3f});
  return csg_union(a, b);
}

static csg_t create_csg_subtract() {
  const auto a =
    csg_cube({.min = {-1.25f, -1.25f, -1.25f}, .max = {0.75f, 0.75f, 0.75f}});
  const auto b = csg_sphere({.center = {0.25f, 0.25f, 0.25f}, .radius = 1.3f});
  return csg_subtract(a, b);
}

static csg_t create_csg_intersect() {
  const auto a =
    csg_cube({.min = {-1.25, -1.25, -1.25}, .max = {0.75f, 0.75f, 0.75f}});
  const auto b = csg_sphere({.center = {0.25f, 0.25f, 0.25f}, .radius = 1.3f});
  return csg_intersect(a, b);
}

static csg_t create_csg_classic() {
  const auto a = csg_cube();
  const auto b = csg_sphere({.radius = 1.35f, .stacks = 12});
  const auto c = csg_cylinder(
    {.radius = 0.7f,
     .start = as::vec3f(-1.0f, 0.0f, 0.0f),
     .end = as::vec3f(1.0f, 0.0f, 0.0f)});
  const auto d = csg_cylinder(
    {.radius = 0.7f,
     .start = as::vec3f(0.0f, -1.0f, 0.0f),
     .end = as::vec3f(0.0f, 1.0f, 0.0f)});
  const auto e = csg_cylinder(
    {.radius = 0.7f,
     .start = as::vec3f(0.0f, 0.0f, -1.0f),
     .end = as::vec3f(0.0f, 0.0f, 1.0f)});
  return csg_subtract(csg_intersect(a, b), csg_union(csg_union(c, d), e));
}

static csg_t create_csg_transform_test() {
  auto a = csg_cube();
  auto b = csg_cube();
  auto c = csg_cube();
  csg_transform_csg_inplace(
    b,
    as::mat4_from_mat3_vec3(
      as::mat3_rotation_z(as::k_half_pi * 0.5f), as::vec3f(1.5f, 0.0f, 0.0f)));
  csg_transform_csg_inplace(
    c, as::mat4_from_mat3_vec3(
         as::mat3_rotation_z(-as::k_half_pi * 0.5f),
         as::vec3f(-1.5f, 0.0f, 0.0f)));
  return csg_subtract(csg_subtract(a, b), c);
}

void csg_scene_t::setup(
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

  target_camera_.pivot = as::vec3(0.0f, 8.0f, -10.0f);
  target_camera_.pitch = as::radians(30.0f);
  camera_ = target_camera_;

  csg_t union_csg = create_csg_union();
  csg_transform_csg_inplace(
    union_csg, as::mat4_from_mat3_vec3(
                 as::mat3_rotation_y(as::k_half_pi), as::vec3f::axis_x(-5.0f)));

  csg_t subtract_csg = create_csg_subtract();
  csg_transform_csg_inplace(
    subtract_csg, as::mat4_from_mat3(as::mat3_rotation_y(as::k_half_pi)));

  csg_t intersect_csg = create_csg_intersect();
  csg_transform_csg_inplace(
    intersect_csg,
    as::mat4_from_mat3_vec3(
      as::mat3_rotation_y(as::k_half_pi), as::vec3f::axis_x(5.0f)));

  render_thing_t::init();
  // render_things_.push_back(render_thing_from_csg(
  //   union_csg, as::mat4f::identity(), as::vec3f(1.0f, 1.0f, 0.0f)));
  // render_things_.push_back(render_thing_from_csg(
  //   subtract_csg, as::mat4f::identity(), as::vec3f(1.0f, 1.0f, 0.0f)));
  // render_things_.push_back(render_thing_from_csg(
  //   intersect_csg, as::mat4f::identity(), as::vec3f(1.0f, 1.0f, 0.0f)));
  // render_things_.push_back(render_thing_from_csg(
  //   create_csg_classic(),
  //   as::mat4_from_mat3_vec3(
  //     as::mat3_rotation_y(as::k_half_pi), as::vec3f::axis_y(5.0f)),
  //   as::vec3f(1.0f, 1.0f, 0.0f)));
  // render_things_.push_back(render_thing_from_csg(
  //   create_csg_transform_test(),
  //   as::mat4_from_vec3(as::vec3f(-5.0f, 5.0f, 0.0f)),
  //   as::vec3f(0.0f, 1.0f, 1.0f)));

  u_light_pos_ = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4, 1);
  u_camera_pos_ =
    bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4, 1);
}

void csg_scene_t::input(const SDL_Event& current_event) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void csg_scene_t::update(debug_draw_t& debug_draw, const float delta_time) {
  ImGui::Begin("CSG");

  ImGui::Checkbox("Wireframe", &wireframe_);
  ImGui::Checkbox("Normals", &normals_);

  if (ImGui::Button("Add shape")) {
    const auto csg_handle = csg_kinds_.add(csg_shape_t{});
    csg_kinds_.call(csg_handle, [this](csg_kind_t& csg_kind) {
      auto* shape = std::get_if<csg_shape_t>(&csg_kind);
      switch (shape->shape) {
        case shape_e::cube: {
          shape->csg = csg_cube();
        } break;
        case shape_e::sphere: {
          shape->csg = csg_sphere();
        } break;
        case shape_e::cylinder: {
          shape->csg = csg_cylinder();
        } break;
      }
      shape->transform = as::mat4_from_vec3(
        camera_.pivot + as::mat3_basis_z(camera_.rotation()) * 10.0f);
      csg_transform_csg_inplace(shape->csg, shape->transform);
      shape->render_thing_handle = render_things_.add(render_thing_from_csg(
        shape->csg, as::mat4f::identity(), as::vec3f(1.0f, 0.0f, 0.0f)));
    });
  }

  if (ImGui::Button("Add operation")) {
    const auto csg_handle = csg_kinds_.add(csg_operation_t{});
    csg_kinds_.call(csg_handle, [](csg_kind_t& csg_kind) {
      auto* operation = std::get_if<csg_operation_t>(&csg_kind);
      operation->operation = csg_union;
    });
  }

  ImGui::Text("CSG primitives");
  for (const auto [i, kind] : ei::enumerate(csg_kinds_)) {
    std::visit(
      overloads{
        [this, i](csg_shape_t& shape) {
          int shape_type = (int)shape.shape;
          ImGui::PushID(i);
          ImGui::Combo("Shape", &shape_type, "Cube\0Sphere\0Cylinder\0");
          if (shape_type != (int)shape.shape) {
            switch (shape_type) {
              case 0: {
                shape.csg = csg_cube();
              } break;
              case 1: {
                shape.csg = csg_sphere();
              } break;
              case 2: {
                shape.csg = csg_cylinder();
              } break;
            }
            csg_transform_csg_inplace(shape.csg, shape.transform);
            render_things_.call(
              shape.render_thing_handle,
              [](const render_thing_t& render_thing) {
                destroy_render_thing(render_thing);
              });
            render_things_.remove(shape.render_thing_handle);
            shape.render_thing_handle =
              render_things_.add(render_thing_from_csg(
                shape.csg, as::mat4f::identity(), as::vec3f(1.0f, 0.0f, 0.0f)));
          }
          shape.shape = (shape_e)shape_type;
          ImGui::InputText("Shape name", shape.name, 64);
          ImGui::PopID();
        },
        [i, this](csg_operation_t& operation) {
          ImGui::PushID(i);
          int operation_type = (int)operation.operation_type;
          ImGui::Combo(
            "Operation", &operation_type, "Union\0Intersection\0Difference\0");
          if (operation_type != (int)operation.operation_type) {
            switch (operation_type) {
              case 0: {
                operation.operation = csg_union;
              } break;
              case 1: {
                operation.operation = csg_intersect;
              } break;
              case 2: {
                operation.operation = csg_subtract;
              } break;
            }
          }
          operation.operation_type = (operation_e)operation_type;
          ImGui::InputText("Name", operation.name, 64);
          ImGui::InputText("LHS", operation.lhs, 64);
          auto lhs_it = std::find_if(
            csg_kinds_.begin(), csg_kinds_.end(),
            [&operation](const csg_kind_t& kind) {
              auto result = std::visit(
                overloads{
                  [&operation](const csg_shape_t& shape) {
                    return std::strlen(shape.name) > 0
                        && shape.name == std::string(operation.lhs);
                  },
                  [&operation](const csg_operation_t& op) {
                    return std::strlen(op.name) > 0
                        && op.name == std::string(operation.lhs);
                  }},
                kind);
              return result;
            });
          ImGui::InputText("RHS", operation.rhs, 64);
          auto rhs_it = std::find_if(
            csg_kinds_.begin(), csg_kinds_.end(),
            [&operation](const csg_kind_t& kind) {
              auto result = std::visit(
                overloads{
                  [&operation](const csg_shape_t& shape) {
                    return std::strlen(shape.name) > 0
                        && shape.name == std::string(operation.rhs);
                  },
                  [&operation](const csg_operation_t& op) {
                    return std::strlen(op.name) > 0
                        && op.name == std::string(operation.rhs);
                  }},
                kind);
              return result;
            });
          const auto lhs_handle = csg_kinds_.handle_from_index(
            std::distance(csg_kinds_.begin(), lhs_it));
          const auto rhs_handle = csg_kinds_.handle_from_index(
            std::distance(csg_kinds_.begin(), rhs_it));
          if (
            lhs_it != csg_kinds_.end() && rhs_it != csg_kinds_.end()
            && lhs_handle != operation.lhs_handle
            && rhs_handle != operation.rhs_handle) {
            // todo - need to handle csg_operation_t here too
            auto* lhs_shape = std::get_if<csg_shape_t>(&*lhs_it);
            auto* rhs_shape = std::get_if<csg_shape_t>(&*rhs_it);
            render_things_.call(
              lhs_shape->render_thing_handle,
              [](const render_thing_t& render_thing) {
                destroy_render_thing(render_thing);
              });
            render_things_.remove(lhs_shape->render_thing_handle);
            render_things_.call(
              rhs_shape->render_thing_handle,
              [](const render_thing_t& render_thing) {
                destroy_render_thing(render_thing);
              });
            render_things_.remove(rhs_shape->render_thing_handle);
            csg_t csg = operation.operation(lhs_shape->csg, rhs_shape->csg);
            operation.render_thing_handle =
              render_things_.add(render_thing_from_csg(
                csg, as::mat4f::identity(), as::vec3f(1.0f, 1.0f, 0.0f)));
            operation.lhs_handle = csg_kinds_.handle_from_index(
              std::distance(csg_kinds_.begin(), lhs_it));
            operation.rhs_handle = csg_kinds_.handle_from_index(
              std::distance(csg_kinds_.begin(), rhs_it));
          }
          if (
            lhs_it == csg_kinds_.end()
              && operation.lhs_handle != thh::handle_t()
              && lhs_handle == thh::handle_t()
            || rhs_it == csg_kinds_.end()
                 && operation.rhs_handle != thh::handle_t()
                 && rhs_handle == thh::handle_t()) {
            render_things_.call(
              operation.render_thing_handle,
              [](const render_thing_t& render_thing) {
                destroy_render_thing(render_thing);
              });
            render_things_.remove(operation.render_thing_handle);
            csg_kinds_.call(
              operation.lhs_handle, [this, &operation](csg_kind_t& kind) {
                auto* lhs_shape = std::get_if<csg_shape_t>(&kind);
                lhs_shape->render_thing_handle =
                  render_things_.add(render_thing_from_csg(
                    lhs_shape->csg, as::mat4f::identity(),
                    as::vec3f(1.0f, 1.0f, 0.0f)));
                operation.lhs_handle = thh::handle_t();
              });
            csg_kinds_.call(
              operation.rhs_handle, [this, &operation](csg_kind_t& kind) {
                auto* rhs_shape = std::get_if<csg_shape_t>(&kind);
                rhs_shape->render_thing_handle =
                  render_things_.add(render_thing_from_csg(
                    rhs_shape->csg, as::mat4f::identity(),
                    as::vec3f(1.0f, 1.0f, 0.0f)));
                operation.rhs_handle = thh::handle_t();
              });
          }
          ImGui::PopID();
        },
      },
      kind);
  }

  ImGui::End();

  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ = asci::smoothCamera(
    camera_, target_camera_, asci::SmoothProps{}, delta_time);

  for (const auto& render_thing : render_things_) {
    render_thing_debug(
      render_thing, *debug_draw.debug_lines,
      {.normals = normals_, .wireframe = wireframe_});
  }

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  drawGrid(*debug_draw.debug_lines, as::vec3::zero());

  const auto offset = as::mat4::identity();

  bgfx::setUniform(u_light_pos_, (void*)&light_pos_, 1);
  bgfx::setUniform(u_camera_pos_, (void*)&camera_.pivot, 1);

  for (const auto& render_thing : render_things_) {
    render_thing_draw(render_thing, main_view_);
  }
}

void csg_scene_t::teardown() {
  bgfx::destroy(u_light_pos_);
  bgfx::destroy(u_camera_pos_);
  for (const auto& render_thing : render_things_) {
    destroy_render_thing(render_thing);
  }
  render_thing_t::deinit();
}
