#include "hierarchy-scene.h"

#include <SDL.h>
#include <imgui.h>

#include "fx.inl"

// https://github.com/ocornut/imgui/issues/3606
void FxTestBed()
{
  const ImGuiIO& io = ImGui::GetIO();
  ImGui::Begin("FX", NULL, ImGuiWindowFlags_AlwaysAutoResize);

  ImVec2 size(320.0f, 180.0f);
  ImGui::SetWindowSize(size);

  if (ImGui::IsWindowCollapsed()) {
    ImGui::End();
    return;
  }

  ImGui::InvisibleButton("canvas", size);
  ImVec2 p0 = ImGui::GetItemRectMin();
  ImVec2 p1 = ImGui::GetItemRectMax();
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->PushClipRect(p0, p1);

  ImVec4 mouse_data;
  mouse_data.x = (io.MousePos.x - p0.x) / size.x;
  mouse_data.y = (io.MousePos.y - p0.y) / size.y;
  mouse_data.z = io.MouseDownDuration[0];
  mouse_data.w = io.MouseDownDuration[1];

  FX(draw_list, p0, p1, size, mouse_data, (float)ImGui::GetTime());
  draw_list->PopClipRect();
  ImGui::End();
}

void imgui_hierarchy_scene_t::setup(
  const bgfx::ViewId main_view, [[maybe_unused]] const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  main_view_ = main_view;

  root_handles = demo::create_sample_entities(entities);
  interaction.select(root_handles.front(), entities, root_handles);
}

void imgui_hierarchy_scene_t::update(
  debug_draw_t& debug_draw, const float delta_time)
{
  // imgui hierarchy experiments
  hy_ig::imguiInteractionDrawListHierarchy(entities, interaction, root_handles);
  hy_ig::imguiInteractionNormalHierarchy(entities, interaction, root_handles);
  hy_ig::imguiOnlyRecursiveHierarchy(entities, root_handles);

  FxTestBed();

  bgfx::touch(main_view_);
}
