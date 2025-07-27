#include "csg-scene.h"

#include "bgfx-helpers.h"
#include "csg/csg.h"
#include "debug.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <easy_iterator.h>
#include <imgui.h>
#include <imgui_stdlib.h>

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

csg_t build_csg(const csg_kind_t& csg_kind, const csg_kinds_t& csg_kinds) {
  return std::visit(
    overloads{
      [](const csg_shape_t& shape) {
        // base case
        return shape.csg;
      },
      [&csg_kinds](const csg_operation_t& operation) {
        const std::optional<const csg_kind_t*> lhs_csg_kind =
          csg_kinds.call_return(
            operation.lhs_handle,
            [](const csg_kind_t& csg_kind) { return &csg_kind; });
        csg_t csg_lhs = build_csg(**lhs_csg_kind, csg_kinds);
        const std::optional<const csg_kind_t*> rhs_csg_kind =
          csg_kinds.call_return(
            operation.rhs_handle,
            [](const csg_kind_t& csg_kind) { return &csg_kind; });
        csg_t csg_rhs = build_csg(**rhs_csg_kind, csg_kinds);
        return operation.operation(csg_lhs, csg_rhs);
      }},
    csg_kind);
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
  render_things_.add(render_thing_from_csg(
    union_csg, as::mat4f::identity(), as::vec3f(1.0f, 1.0f, 0.0f)));
  render_things_.add(render_thing_from_csg(
    subtract_csg, as::mat4f::identity(), as::vec3f(1.0f, 1.0f, 0.0f)));
  render_things_.add(render_thing_from_csg(
    intersect_csg, as::mat4f::identity(), as::vec3f(1.0f, 1.0f, 0.0f)));
  render_things_.add(render_thing_from_csg(
    create_csg_classic(),
    as::mat4_from_mat3_vec3(
      as::mat3_rotation_y(as::k_half_pi), as::vec3f::axis_y(5.0f)),
    as::vec3f(1.0f, 1.0f, 0.0f)));
  render_things_.add(render_thing_from_csg(
    create_csg_transform_test(),
    as::mat4_from_vec3(as::vec3f(-5.0f, 5.0f, 0.0f)),
    as::vec3f(0.0f, 1.0f, 1.0f)));

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
    const auto csg_handle = root_csg_kinds_.add(csg_shape_t{});
    root_csg_kinds_.call(csg_handle, [this](csg_kind_t& csg_kind) {
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
    const auto csg_handle = root_csg_kinds_.add(csg_operation_t{});
    root_csg_kinds_.call(csg_handle, [](csg_kind_t& csg_kind) {
      auto* operation = std::get_if<csg_operation_t>(&csg_kind);
      operation->operation = csg_union;
    });
  }

  std::vector<thh::handle_t> handles_to_remove;
  std::vector<csg_kind_t> csgs_to_restore;

  const auto cleanup_render_thing = [this](thh::handle_t& render_thing_handle) {
    render_things_.call(
      render_thing_handle, [](const render_thing_t& render_thing) {
        destroy_render_thing(render_thing);
      });
    render_things_.remove(render_thing_handle);
    render_thing_handle = thh::handle_t();
  };

  ImGui::Text("CSG root primitives");
  for (const auto [i, kind] : ei::enumerate(root_csg_kinds_)) {
    std::visit(
      overloads{
        [this, i, &cleanup_render_thing](csg_shape_t& shape) {
          auto shape_type = static_cast<int>(shape.shape);
          ImGui::PushID(i);
          ImGui::Combo("Shape", &shape_type, "Cube\0Sphere\0Cylinder\0");
          if (shape_type != static_cast<int>(shape.shape)) {
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
            // destroy existing render_thing when changing shape
            cleanup_render_thing(shape.render_thing_handle);
            // add new render_thing after building csg
            shape.render_thing_handle =
              render_things_.add(render_thing_from_csg(
                shape.csg, as::mat4f::identity(), as::vec3f(1.0f, 0.0f, 0.0f)));
          }
          shape.shape = (shape_e)shape_type;
          ImGui::InputText("Shape name", &shape.name);
          ImGui::PopID();
        },
        [this, i, &handles_to_remove, &csgs_to_restore,
         &cleanup_render_thing](csg_operation_t& operation) {
          ImGui::PushID(i);
          auto operation_type = static_cast<int>(operation.operation_type);
          ImGui::Combo(
            "Operation", &operation_type, "Union\0Intersection\0Difference\0");
          if (operation_type != static_cast<int>(operation.operation_type)) {
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
          operation.operation_type = static_cast<operation_e>(operation_type);
          ImGui::InputText("Name", &operation.name);
          ImGui::InputText("LHS", &operation.lhs_name);
          ImGui::InputText("RHS", &operation.rhs_name);

          const auto find_csg_by_name =
            [this](const std::string& name, csg_kinds_t& csg_kinds) {
              return std::find_if(
                csg_kinds.begin(), csg_kinds.end(),
                [&name](const csg_kind_t& kind) {
                  return std::visit(
                    [&name](const auto& shape_or_op) {
                      return !shape_or_op.name.empty()
                          && shape_or_op.name == name;
                    },
                    kind);
                });
            };

          auto root_lhs_it =
            find_csg_by_name(operation.lhs_name, root_csg_kinds_);
          auto root_rhs_it =
            find_csg_by_name(operation.rhs_name, root_csg_kinds_);

          const auto lhs_handle = root_csg_kinds_.handle_from_index(
            std::distance(root_csg_kinds_.begin(), root_lhs_it));
          const auto rhs_handle = root_csg_kinds_.handle_from_index(
            std::distance(root_csg_kinds_.begin(), root_rhs_it));
          // found handles in root_csg_kinds_ - merge
          if (
            root_lhs_it != root_csg_kinds_.end()
            && root_rhs_it != root_csg_kinds_.end()
            && lhs_handle != operation.lhs_handle
            && rhs_handle != operation.rhs_handle) {

            auto* lhs_shape = std::get_if<csg_shape_t>(&*root_lhs_it);
            auto* lhs_operation = std::get_if<csg_operation_t>(&*root_lhs_it);
            auto* rhs_shape = std::get_if<csg_shape_t>(&*root_rhs_it);
            auto* rhs_operation = std::get_if<csg_operation_t>(&*root_rhs_it);

            thh::handle_t unused_handle;
            auto& lhs_render_thing_handle =
              lhs_shape       ? lhs_shape->render_thing_handle
              : lhs_operation ? lhs_operation->render_thing_handle
                              : unused_handle;
            auto& rhs_render_thing_handle =
              rhs_shape       ? rhs_shape->render_thing_handle
              : rhs_operation ? rhs_operation->render_thing_handle
                              : unused_handle;

            // remove current left and right render_thing if they exist
            cleanup_render_thing(lhs_render_thing_handle);
            cleanup_render_thing(rhs_render_thing_handle);

            const auto current_csg_handle =
              root_csg_kinds_.handle_from_index(i);

            const auto kind_from_handle = [this](const thh::handle_t handle) {
              return root_csg_kinds_
                .call_return(
                  handle, [](const csg_kind_t& csg_kind) { return csg_kind; })
                .value();
            };

            csg_kind_t lhs_csg_kind = kind_from_handle(lhs_handle);
            csg_kind_t rhs_csg_kind = kind_from_handle(rhs_handle);

            // remove from root_csg_kinds_
            handles_to_remove.push_back(lhs_handle);
            handles_to_remove.push_back(rhs_handle);

            // move to child_csg_kinds_
            const auto next_lhs_handle = child_csg_kinds_.add(lhs_csg_kind);
            const auto next_rhs_handle = child_csg_kinds_.add(rhs_csg_kind);

            operation.lhs_handle = next_lhs_handle;
            operation.rhs_handle = next_rhs_handle;

            csg_t csg = build_csg(operation, child_csg_kinds_);

            operation.render_thing_handle =
              render_things_.add(render_thing_from_csg(
                csg, as::mat4f::identity(), as::vec3f(1.0f, 1.0f, 0.0f)));
          }
          auto child_lhs_it =
            find_csg_by_name(operation.lhs_name, child_csg_kinds_);
          auto child_rhs_it =
            find_csg_by_name(operation.rhs_name, child_csg_kinds_);
          // if either lhs or rhs names are cleared from an operation, restore
          // lhs and rhs to root
          if (
            (child_lhs_it == child_csg_kinds_.end()
             && operation.lhs_handle != thh::handle_t())
            || (child_rhs_it == child_csg_kinds_.end() && operation.rhs_handle != thh::handle_t())) {
            const auto extract_and_remove = [this](thh::handle_t& handle) {
              csg_kind_t csg_kind =
                child_csg_kinds_
                  .call_return(
                    handle, [](const csg_kind_t& csg_kind) { return csg_kind; })
                  .value();
              child_csg_kinds_.remove(handle);
              handle = thh::handle_t();
              return csg_kind;
            };

            csg_kind_t lhs_csg_kind = extract_and_remove(operation.lhs_handle);
            csg_kind_t rhs_csg_kind = extract_and_remove(operation.rhs_handle);

            cleanup_render_thing(operation.render_thing_handle);

            for (auto& csg_kind :
                 {std::ref(lhs_csg_kind), std::ref(rhs_csg_kind)}) {
              std::visit(
                [this, &csg_kind](auto& shape_or_op) {
                  shape_or_op.render_thing_handle =
                    render_things_.add(render_thing_from_csg(
                      build_csg(csg_kind, child_csg_kinds_),
                      as::mat4f::identity(), as::vec3f(1.0f, 0.0f, 0.0f)));
                },
                csg_kind.get());
            }

            csgs_to_restore.push_back(lhs_csg_kind);
            csgs_to_restore.push_back(rhs_csg_kind);
          }
          ImGui::PopID();
        },
      },
      kind);
  }

  // show unavailable shapes/operations that are being consumed by root
  // operations
  ImGui::Text("CSG child primitives (in use)");
  ImGui::BeginDisabled();
  for (const auto [i, kind] : ei::enumerate(child_csg_kinds_)) {
    std::visit(
      overloads{
        [this, i](csg_shape_t& shape) {
          ImGui::PushID(i + root_csg_kinds_.size());
          auto shape_type = static_cast<int>(shape.shape);
          ImGui::Combo("Shape", &shape_type, "Cube\0Sphere\0Cylinder\0");
          ImGui::InputText("Shape name", &shape.name);
          ImGui::PopID();
        },
        [this, i](csg_operation_t& operation) {
          ImGui::PushID(i + root_csg_kinds_.size());
          int operation_type = static_cast<int>(operation.operation_type);
          ImGui::Combo(
            "Operation", &operation_type, "Union\0Intersection\0Difference\0");
          ImGui::InputText("Name", &operation.name);
          ImGui::InputText("LHS", &operation.lhs_name);
          ImGui::InputText("RHS", &operation.rhs_name);
          ImGui::PopID();
        }},
      kind);
  }
  ImGui::EndDisabled();

  for (const auto handle : handles_to_remove) {
    root_csg_kinds_.remove(handle);
  }
  handles_to_remove.clear();

  for (const auto& csg_kind : csgs_to_restore) {
    root_csg_kinds_.add(csg_kind);
  }
  csgs_to_restore.clear();

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
