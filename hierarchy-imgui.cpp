#include "hierarchy-imgui.h"

#include <imgui.h>

namespace hy_ig
{

void imguiInteractionDrawListHierarchy(
  const thh::handle_vector_t<hy::entity_t>& entities,
  hy::interaction_t& interaction,
  const std::vector<thh::handle_t>& root_handles)
{
  ImGui::Begin("Hierarchy Interaction Draw List");

  if (ImGui::IsWindowCollapsed()) {
    ImGui::End();
    return;
  }

  ImVec2 pos = ImGui::GetWindowPos();
  ImVec2 size = ImGui::GetWindowSize();
  ImGui::InvisibleButton("canvas", size);
  ImVec2 p0 = ImGui::GetWindowContentRegionMin();
  ImVec2 p1 = ImGui::GetWindowContentRegionMax();

  p0.x += pos.x;
  p0.y += pos.y;
  p1.x += pos.x;
  p1.y += pos.y;

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->PushClipRect(p0, p1);

  const auto display_name = [draw_list, p0](const hy::display_info_t& di) {
    const auto entry = std::string("|-- ") + di.name;
    const ImU32 col = di.selected   ? 0xffffff00
                    : !di.collapsed ? 0xffffffff
                                    : 0xff00ffff;
    draw_list->AddText(
      ImVec2(p0.x + float(di.indent * 28), p0.y + float(di.level * 12)), col,
      entry.c_str());
  };

  const auto display_connection = [draw_list, p0](int level, int indent) {
    draw_list->AddText(
      ImVec2(p0.x + float(indent * 28), p0.y + float(level * 12)), 0xffffffff, "|");
  };

  hy::display_hierarchy(
    entities, interaction, root_handles, display_name, [] {},
    display_connection);

  draw_list->PopClipRect();
  ImGui::End();
}

void imguiInteractionNormalHierarchy(
  const thh::handle_vector_t<hy::entity_t>& entities,
  hy::interaction_t& interaction,
  const std::vector<thh::handle_t>& root_handles)
{
  ImGui::Begin("Hierarchy Interaction Normal");

  if (ImGui::IsWindowCollapsed()) {
    ImGui::End();
    return;
  }

  const auto display = [&interaction, &entities,
                        &root_handles](const hy::display_info_t& di) {
    if (di.has_children) {
      ImGui::SetNextItemOpen(!di.collapsed);
      bool open = ImGui::TreeNodeEx(di.name.c_str());
      if (ImGui::IsItemClicked()) {
        if (open) {
          interaction.expand(di.entity_handle);
        } else {
          interaction.collapse(di.entity_handle, entities);
        }
      }
    } else {
      ImGui::Text("%s", di.name.c_str());
    }
    if (ImGui::IsItemHovered()) {
      interaction.select(di.entity_handle, entities, root_handles);
    }
  };

  const auto display_pop = []() { ImGui::TreePop(); };

  const auto display_connection = [](int level, int indent) {};

  interaction.select(thh::handle_t(-1, -1), entities, root_handles);

  hy::display_hierarchy(
    entities, interaction, root_handles, display, display_pop,
    display_connection);

  ImGui::End();
}

static void imguiOnlyRecursiveHierarchyInternal(
  const thh::handle_vector_t<hy::entity_t>& entities,
  const std::vector<thh::handle_t>& handles)
{
  for (const auto handle : handles) {
    entities.call(handle, [&](const auto& entity) {
      const auto& children = entity.children_;
      if (!children.empty()) {
        if (ImGui::TreeNodeEx(entity.name_.c_str())) {
          imguiOnlyRecursiveHierarchyInternal(entities, children);
          ImGui::TreePop();
        }
      } else {
        ImGui::Text("%s", entity.name_.c_str());
      }
    });
  }
}

void imguiOnlyRecursiveHierarchy(
  const thh::handle_vector_t<hy::entity_t>& entities,
  const std::vector<thh::handle_t>& handles)
{
  ImGui::Begin("Hierarchy Recursive");
  imguiOnlyRecursiveHierarchyInternal(entities, handles);
  ImGui::End();
}

} // namespace hy_ig
