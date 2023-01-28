#include "marching-cube-scene.h"

#include "bgfx-helpers.h"
#include "file-ops.h"
#include "marching-cubes/marching-cubes.h"

#include <SDL.h>
#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-math-ops.hpp>
#include <as/as-view.hpp>
#include <bx/timer.h>
#include <imgui.h>
#include <thh-bgfx-debug/debug-cube.hpp>
#include <thh-bgfx-debug/debug-line.hpp>
#include <thh-bgfx-debug/debug-quad.hpp>
#include <thh-bgfx-debug/debug-sphere.hpp>

struct PosColorVertex
{
  as::vec3 position_;
  uint32_t abgr_;
};

struct PosNormalVertex
{
  as::vec3 position_;
  as::vec3 normal_;
};

static const PosColorVertex CubeVerticesCol[] = {
  {as::vec3{-1.0f, 1.0f, 1.0f}, 0xff000000},
  {as::vec3{1.0f, 1.0f, 1.0f}, 0xff0000ff},
  {as::vec3{-1.0f, -1.0f, 1.0f}, 0xff00ff00},
  {as::vec3{1.0f, -1.0f, 1.0f}, 0xff00ffff},
  {as::vec3{-1.0f, 1.0f, -1.0f}, 0xffff0000},
  {as::vec3{1.0f, 1.0f, -1.0f}, 0xffff00ff},
  {as::vec3{-1.0f, -1.0f, -1.0f}, 0xffffff00},
  {as::vec3{1.0f, -1.0f, -1.0f}, 0xffffffff},
};

static const uint16_t CubeTriListCol[] = {
  0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7, 0, 2, 4, 4, 2, 6,
  1, 5, 3, 5, 7, 3, 0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7,
};

void marching_cube_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  screen_dimension = as::vec2i(width, height);

  main_view_ = main_view;
  ortho_view_ = ortho_view;

  // size and placement of gizmo on screen
  const float gizmo_offset_percent = 0.85f;
  const float gizmo_size_percent = 0.1f;
  bgfx::setViewClear(gizmo_view_, BGFX_CLEAR_DEPTH);
  bgfx::setViewRect(
    gizmo_view_, uint16_t(float(width) * gizmo_offset_percent),
    uint16_t(float(height) * gizmo_offset_percent),
    uint16_t(float(width) * gizmo_size_percent),
    uint16_t(float(height) * gizmo_size_percent));

  perspective_projection = as::perspective_direct3d_lh(
    as::radians(35.0f), float(width) / float(height), 0.01f, 100.0f);

  pos_col_vert_layout.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();

  pos_norm_vert_layout.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
    .end();

  cube_col_vbh = bgfx::createVertexBuffer(
    bgfx::makeRef(CubeVerticesCol, sizeof(CubeVerticesCol)),
    pos_col_vert_layout);
  cube_col_ibh = bgfx::createIndexBuffer(
    bgfx::makeRef(CubeTriListCol, sizeof(CubeTriListCol)));

  program_norm =
    createShaderProgram("shader/next/v_next.bin", "shader/next/f_next.bin")
      .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));

  program_col = createShaderProgram(
                  "shader/simple/v_simple.bin", "shader/simple/f_simple.bin")
                  .value_or(bgfx::ProgramHandle(BGFX_INVALID_HANDLE));

  u_light_dir = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4, 1);
  u_camera_pos = bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4, 1);

  asci::Cameras& cameras = camera_system.cameras_;
  cameras.addCamera(&first_person_rotate_camera);
  cameras.addCamera(&first_person_pan_camera);
  cameras.addCamera(&first_person_translate_camera);
  cameras.addCamera(&first_person_wheel_camera);

  points = mc::createPointVolume(dimension, 10000.0f);
  cell_values = mc::createCellValues(dimension);
  cell_positions = mc::createCellPositions(dimension);

  scene_alias = (int*)&scene;
}

void marching_cube_scene_t::input(const SDL_Event& current_event)
{
  camera_system.handleEvents(asci_sdl::sdlToInput(&current_event));
}

void marching_cube_scene_t::update(
  debug_draw_t& debug_draw, const float delta_time)
{
  auto freq = double(bx::getHPFrequency());
  int64_t time_window = fps::calculateWindow(fps, bx::getHPCounter());
  double framerate = time_window > -1 ? (double)(fps.MaxSamples - 1)
                                          / (double(time_window) / freq)
                                      : 0.0;

  target_camera = camera_system.stepCamera(target_camera, delta_time);
  camera =
    asci::smoothCamera(camera, target_camera, asci::SmoothProps{}, delta_time);

  // marching cube scene
  {
    float view[16];
    as::mat_to_arr(as::mat4_from_affine(camera.view()), view);

    float proj[16];
    as::mat_to_arr(perspective_projection, proj);

    bgfx::setViewTransform(main_view_, view, proj);

    auto marching_cube_begin = bx::getHPCounter();

    static bool analytical_normals = true;
    static bool draw_normals = false;

    const as::mat3 cam_orientation = camera.transform().rotation;
    static float camera_adjust_noise = (float(dimension) * 0.5f) + 1.0f;
    static float camera_adjust_sphere = 50.0f;

    const as::vec3 lookat = camera.pivot;

    static float tesselation = 1.0f;
    static float scale = 14.0f;
    static float threshold = 4.0f; // initial

    switch (scene) {
      case Scene::Noise: {
        const as::vec3 offset =
          lookat + cam_orientation * as::vec3::axis_z(camera_adjust_noise);
        generatePointData(points, dimension, scale, tesselation, offset);
      } break;
        break;
      case Scene::Sphere: {
        int x;
        int y;
        SDL_GetMouseState(&x, &y);
        const auto orientation = as::affine_inverse(camera.view()).rotation;
        const auto world_position = as::screen_to_world(
          as::vec2i(x, y), perspective_projection, camera.view(),
          screen_dimension, as::vec2(0.0f, 1.0f));
        const auto ray_origin = camera.translation();
        const auto ray_direction =
          as::vec_normalize(world_position - ray_origin);
        const as::vec3 offset =
          lookat + cam_orientation * as::vec3::axis_z(camera_adjust_sphere);
        generatePointData(
          points, dimension, tesselation, offset, ray_origin, ray_direction,
          50.0f);
      } break;
    }

    generateCellData(cell_positions, cell_values, points, dimension);

    const auto triangles =
      mc::march(cell_positions, cell_values, dimension, threshold);

    std::vector<as::index> indices;
    indices.resize(triangles.size() * 3);

    as::index index = 0;
    as::index unique = 0;
    for (const auto& tri : triangles) {
      for (int64_t i = 0; i < 3; ++i) {
        const auto vert = tri.verts_[i];
        const auto norm = tri.norms_[i];
        const auto exists = unique_verts.find(vert);
        if (exists == std::end(unique_verts)) {
          filtered_verts.push_back(vert);
          filtered_norms.push_back(norm);
          unique_verts.insert({vert, unique});
          indices[index] = unique;
          unique++;
        } else {
          indices[index] = exists->second;
        }
        index++;
      }
    }

    uint32_t max_vertices = 32 << 10;
    const auto available_vertex_count =
      bgfx::getAvailTransientVertexBuffer(max_vertices, pos_norm_vert_layout);

    const auto available_index_count =
      bgfx::getAvailTransientIndexBuffer(max_vertices);

    if (
      available_vertex_count == max_vertices
      && available_index_count == max_vertices) {

      bgfx::TransientVertexBuffer mc_triangle_tvb;
      bgfx::allocTransientVertexBuffer(
        &mc_triangle_tvb, max_vertices, pos_norm_vert_layout);

      bgfx::TransientIndexBuffer tib;
      bgfx::allocTransientIndexBuffer(&tib, max_vertices);

      auto* vertex = (PosNormalVertex*)mc_triangle_tvb.data;
      auto* index_data = (int16_t*)tib.data;

      for (as::index i = 0; i < filtered_verts.size(); i++) {
        vertex[i].normal_ = analytical_normals
                            ? as::vec_normalize(filtered_norms[i])
                            : as::vec3::zero();
        vertex[i].position_ = filtered_verts[i];
      }

      for (as::index indice = 0; indice < indices.size(); indice++) {
        index_data[indice] = indices[indice];
      }

      if (!analytical_normals) {
        for (as::index indice = 0; indice < indices.size(); indice += 3) {
          const as::vec3 e1 = filtered_verts[indices[indice]]
                            - filtered_verts[indices[indice + 1]];
          const as::vec3 e2 = filtered_verts[indices[indice + 2]]
                            - filtered_verts[indices[indice + 1]];
          const as::vec3 normal = as::vec3_cross(e1, e2);

          vertex[indices[indice]].normal_ += normal;
          vertex[indices[indice + 1]].normal_ += normal;
          vertex[indices[indice + 2]].normal_ += normal;
        }

        for (as::index i = 0; i < filtered_verts.size(); i++) {
          vertex[i].normal_ = as::vec_normalize(vertex[i].normal_);
        }
      }

      float model[16];
      as::mat_to_arr(as::mat4::identity(), model);
      bgfx::setTransform(model);

      bgfx::setUniform(u_light_dir, (void*)&light_dir, 1);
      bgfx::setUniform(u_camera_pos, (void*)&camera.pivot, 1);

      bgfx::setIndexBuffer(&tib, 0, indices.size());
      bgfx::setVertexBuffer(0, &mc_triangle_tvb, 0, filtered_verts.size());
      bgfx::setState(BGFX_STATE_DEFAULT);
      bgfx::submit(main_view_, program_norm);

      if (draw_normals) {
        for (as::index i = 0; i < filtered_verts.size(); i++) {
          debug_draw.debug_lines->addLine(
            vertex[i].position_, vertex[i].position_ + vertex[i].normal_,
            0xff000000);
        }
      }
    }

    bgfx::touch(main_view_);

    const double to_ms = 1000.0 / freq;
    auto marching_cube_time = double(bx::getHPCounter() - marching_cube_begin);

    ImGui::Begin("Marching Cubes");
    ImGui::Text("Framerate: ");
    ImGui::SameLine(100);
    ImGui::Text("%f", framerate);

    ImGui::Text("Marching Cube update: ");
    ImGui::SameLine(160);
    ImGui::Text("%f", marching_cube_time * to_ms);

    float light_dir_arr[3];
    as::vec_to_arr(light_dir, light_dir_arr);
    ImGui::InputFloat3("Light Dir", light_dir_arr);
    light_dir = as::vec_from_arr(light_dir_arr);

    ImGui::SliderFloat("Threshold", &threshold, 0.0f, 10.0f);
    ImGui::SliderFloat("Back Noise", &camera_adjust_noise, 0.0f, 100.0f);
    ImGui::SliderFloat("Scale", &scale, 0.0f, 100.0f);
    ImGui::SliderFloat("Tesselation", &tesselation, 0.001f, 10.0f);
    ImGui::Checkbox("Draw Normals", &draw_normals);
    ImGui::Checkbox("Analytical Normals", &analytical_normals);
    static const char* scenes[] = {"Noise", "Sphere"};
    ImGui::Combo(
      "Marching Cubes Scene", scene_alias, scenes, std::size(scenes));
    ImGui::End();
  }

  // gizmo cube
  {
    float view[16];
    as::mat_to_arr(
      as::mat4_from_mat3_vec3(camera.view().rotation, as::vec3::axis_z(10.0f)),
      view);

    const float extent_y = 10.0f;
    const float extent_x =
      extent_y * (screen_dimension.x / float(screen_dimension.y));

    float proj[16];
    as::mat_to_arr(
      as::ortho_direct3d_lh(
        -extent_x, extent_x, -extent_y, extent_y, 0.01f, 100.0f),
      proj);

    bgfx::setViewTransform(gizmo_view_, view, proj);

    as::mat4 rot = as::mat4_from_mat3(as::mat3_scale(4.0f));

    float model[16];
    as::mat_to_arr(rot, model);

    bgfx::setTransform(model);

    bgfx::setVertexBuffer(0, cube_col_vbh);
    bgfx::setIndexBuffer(cube_col_ibh);

    bgfx::submit(gizmo_view_, program_col);
  }

  filtered_verts.clear();
  filtered_norms.clear();
  unique_verts.clear();
}

void marching_cube_scene_t::teardown()
{
  mc::destroyCellValues(cell_values, dimension);
  mc::destroyCellPositions(cell_positions, dimension);
  mc::destroyPointVolume(points, dimension);

  bgfx::destroy(u_camera_pos);
  bgfx::destroy(u_light_dir);
  bgfx::destroy(cube_col_vbh);
  bgfx::destroy(cube_col_ibh);
  bgfx::destroy(program_norm);
  bgfx::destroy(program_col);
}
