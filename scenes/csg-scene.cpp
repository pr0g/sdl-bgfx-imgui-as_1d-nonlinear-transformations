#include "csg-scene.h"

#include "csg/csg.h"
#include "debug.h"

#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>
#include <imgui.h>

#include <thh-bgfx-debug/debug-color.hpp>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-line.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>

#include <ranges>

static csg_t create_csg() {

  const auto csg_cube_1 = csg_cube(
    csg_cube_config_t{
      .min = as::vec3f(-5.0f, -10.0f, -2.5f),
      .max = as::vec3f(5.0f, 10.0f, 2.5f),
      .orientation = as::mat3_rotation_x(as::k_half_pi * 0.5f)});
  // return csg_cube_1;

  // csg_sphere(as::vec3f::zero(), 1.0f);
  const auto csg_cube_2 =
    // csg_cube(as::vec3f::zero(), as::vec3f(2.5f, 0.2f, 0.2f));
    csg_sphere(
      csg_sphere_config_t{
        .position = as::vec3f(0.0f, 3.0f, -0.5f), .radius = 1.5f});

  const auto csg_cube_3a = csg_cube(
    csg_cube_config_t{
      .min = as::vec3f(-5.0f, -5.0f, 0.0f),
      .max = as::vec3f(5.0f, 5.0f, 10.0f),
      .orientation = as::mat3_rotation_z(as::k_half_pi * 0.5f)});

  return csg_union(csg_union(csg_cube_1, csg_cube_2), csg_cube_3a);

  const auto csg_cube_3 =
    // csg_cube(as::vec3f::zero(), as::vec3f(2.5f, 0.2f, 0.2f));
    csg_sphere(
      csg_sphere_config_t{
        .position = as::vec3f(-0.5, 0.0, -0.5), .radius = 0.8f});
  const auto temp =
    csg_subtract(csg_cylinder(csg_cylinder_config_t{}), csg_cube_2);
  // const auto csg_result =
  //   csg_subtract(csg_cylinder(), csg_cube_1); // csg_subtract(temp,
  //   csg_cube_3);
  // const auto polygons = temp.polygons;
  // csg_subtract(csg_subtract(temp, csg_cube_3), csg_cube_1).polygons;
  // csg_subtract(csg_cylinder(csg_cylinder_config_t{}), csg_cube_1)
  //   .polygons; // csg_subtract(temp, csg_cube_3);

  const auto a = csg_cube();
  const auto b = csg_cylinder();
  const auto c = csg_cylinder(
    {.start = {-as::vec3f::axis_x()}, .end = {as::vec3f::axis_x()}});
  const auto d = csg_cylinder(
    {.start = {-as::vec3f::axis_z()}, .end = {as::vec3f::axis_z()}});
  auto inverse = csg_inverse(csg_union(csg_union(b, c), d));

  return inverse;
}

static void build_mesh_from_csg(
  const csg_t csg, std::vector<PosNormalVertex>& csg_vertices,
  std::vector<uint16_t>& csg_indices, lines_indices_t& csg_lines) {
  std::unordered_map<csg_vertex_t, int, csg_vertex_hash_t, csg_vertex_equals_t>
    indexer;
  for (const csg_polygon_t& polygon : csg.polygons) {
    std::vector<int> indices;
    for (const csg_vertex_t vertex : polygon.vertices) {
      if (const auto index = indexer.find(vertex); index == indexer.end()) {
        indices.push_back(indexer.size());
        indexer.insert({vertex, indexer.size()});
        csg_vertices.push_back(
          PosNormalVertex{.position_ = vertex.pos, .normal_ = vertex.normal});
      } else {
        indices.push_back(index->second);
      }
    }
    for (int i = 2; i < indices.size(); i++) {
      csg_indices.push_back(indices[0]);
      csg_indices.push_back(indices[i - 1]);
      csg_indices.push_back(indices[i]);
      csg_lines.push_back({indices[0], indices[i - 1]});
      csg_lines.push_back({indices[i - 1], indices[i]});
      csg_lines.push_back({indices[i], indices[0]});
    }
  }
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
  cameras.addCamera(&orbit_rotate_camera_);
  cameras.addCamera(&orbit_dolly_camera_);

  target_camera_.offset = as::vec3(0.0f, 0.0f, -10.0f);
  target_camera_.pitch = as::radians(30.0f);
  // target_camera_.yaw = as::radians(45.0f);
  camera_ = target_camera_;

  pos_norm_vert_layout_.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
    .end();

  build_mesh_from_csg(create_csg(), csg_vertices_, csg_indices_, csg_lines_);

  csg_norm_vbh_ = bgfx::createVertexBuffer(
    bgfx::makeRef(
      csg_vertices_.data(), csg_vertices_.size() * sizeof(PosNormalVertex)),
    pos_norm_vert_layout_);
  csg_norm_ibh_ = bgfx::createIndexBuffer(
    bgfx::makeRef(csg_indices_.data(), csg_indices_.size() * sizeof(uint16_t)));

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
  u_model_color_ =
    bgfx::createUniform("u_modelColor", bgfx::UniformType::Vec4, 1);
}

void csg_scene_t::input(const SDL_Event& current_event) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  camera_system_.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void csg_scene_t::update(debug_draw_t& debug_draw, const float delta_time) {
  target_camera_ = camera_system_.stepCamera(target_camera_, delta_time);
  camera_ = asci::smoothCamera(
    camera_, target_camera_, asci::SmoothProps{}, delta_time);

  const float adjustment = 0.005f;
  for (const auto line : csg_lines_) {
    debug_draw.debug_lines->addLine(
      csg_vertices_[line.begin].position_
        + csg_vertices_[line.begin].normal_ * adjustment,
      csg_vertices_[line.end].position_
        + csg_vertices_[line.end].normal_ * adjustment,
      0xffaaaaaa);
  }

  float view[16];
  as::mat_to_arr(as::mat4_from_affine(camera_.view()), view);
  float proj[16];
  as::mat_to_arr(perspective_projection_, proj);

  bgfx::setViewTransform(main_view_, view, proj);

  drawGrid(*debug_draw.debug_lines, as::vec3::zero());

  const auto offset = as::mat4::identity();

  float model[16];
  as::mat_to_arr(offset, model);

  bgfx::setTransform(model);

  bgfx::setUniform(u_light_pos_, (void*)&light_pos_, 1);
  bgfx::setUniform(u_camera_pos_, (void*)&camera_.pivot, 1);
  bgfx::setUniform(u_model_color_, (void*)&model_color_, 1);

  bgfx::setVertexBuffer(0, csg_norm_vbh_);
  bgfx::setIndexBuffer(csg_norm_ibh_);
  bgfx::setState(
    BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z
    | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_CULL_CCW);
  bgfx::submit(main_view_, program_norm_);
}

void csg_scene_t::teardown() {
  bgfx::destroy(u_light_pos_);
  bgfx::destroy(u_camera_pos_);
  bgfx::destroy(u_model_color_);
  bgfx::destroy(program_norm_);
  bgfx::destroy(csg_norm_ibh_);
  bgfx::destroy(csg_norm_vbh_);
}
