#include "hierarchy-imgui.h"

#include "imgui.h"

namespace hy_ig
{

void imgui_interaction_draw_list_hierarchy(
  const thh::container_t<hy::entity_t>& entities,
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

  const auto display_name = [draw_list, p0](
                              int level, int indent, thh::handle_t,
                              bool selected, bool collapsed, bool has_children,
                              const std::string& name) {
    const auto entry = std::string("|-- ") + name;
    const ImU32 col = selected   ? 0xffffff00
                    : !collapsed ? 0xffffffff
                                 : 0xff00ffff;
    draw_list->AddText(
      ImVec2(p0.x + indent * 28, p0.y + level * 12), col, entry.c_str());
  };

  const auto display_connection = [draw_list, p0](int level, int indent) {
    draw_list->AddText(
      ImVec2(p0.x + indent * 28, p0.y + level * 12), 0xffffffff, "|");
  };

  hy::display_hierarchy(
    entities, interaction, root_handles, display_name, [] {},
    display_connection);

  draw_list->PopClipRect();
  ImGui::End();
}

void imgui_interaction_normal_hierarchy(
  const thh::container_t<hy::entity_t>& entities,
  hy::interaction_t& interaction,
  const std::vector<thh::handle_t>& root_handles)
{
  ImGui::Begin("Hierarchy Interaction Normal");

  const auto display =
    [&interaction](
      int level, int indent, const thh::handle_t entity_handle, bool selected,
      bool collapsed, bool has_children, const std::string& name) {
      if (has_children) {
        ImGui::SetNextItemOpen(!collapsed);
        bool open = ImGui::TreeNodeEx(name.c_str());
        if (ImGui::IsItemClicked()) {
          if (open) {
            interaction.collapsed_.erase(
              std::remove(
                interaction.collapsed_.begin(), interaction.collapsed_.end(),
                entity_handle),
              interaction.collapsed_.end());
          } else {
            interaction.collapsed_.push_back(entity_handle);
          }
        }
      } else {
        ImGui::Text("%s", name.c_str());
      }
      if (ImGui::IsItemHovered()) {
        interaction.selected_ = entity_handle;
      }
    };

  const auto display_pop = []() { ImGui::TreePop(); };

  const auto display_connection = [](int level, int indent) {};

  interaction.selected_ = thh::handle_t(-1, -1);
  hy::display_hierarchy(
    entities, interaction, root_handles, display, display_pop,
    display_connection);

  ImGui::End();
}

static void imgui_only_recursive_hierarchy_internal(
  const thh::container_t<hy::entity_t>& entities,
  const std::vector<thh::handle_t>& handles)
{
  for (const auto handle : handles) {
    entities.call(handle, [&](const auto& entity) {
      const auto& children = entity.children_;
      if (!children.empty()) {
        if (ImGui::TreeNodeEx(entity.name_.c_str())) {
          imgui_only_recursive_hierarchy_internal(entities, children);
          ImGui::TreePop();
        }
      } else {
        ImGui::Text("%s", entity.name_.c_str());
      }
    });
  }
}

void imgui_only_recursive_hierarchy(
  const thh::container_t<hy::entity_t>& entities,
  const std::vector<thh::handle_t>& handles)
{
  ImGui::Begin("Hierarchy Recursive");
  imgui_only_recursive_hierarchy_internal(entities, handles);
  ImGui::End();
}

} // namespace hy_ig
