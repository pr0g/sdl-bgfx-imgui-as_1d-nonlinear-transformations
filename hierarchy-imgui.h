#pragma once

#include "hierarchy/entity.hpp"

namespace hy_ig
{

void imgui_interaction_draw_list_hierarchy(
  const thh::container_t<hy::entity_t>& entities,
  hy::interaction_t& interaction,
  const std::vector<thh::handle_t>& root_handles);

void imgui_interaction_normal_hierarchy(
  const thh::container_t<hy::entity_t>& entities,
  hy::interaction_t& interaction,
  const std::vector<thh::handle_t>& root_handles);

void imgui_only_recursive_hierarchy(
  const thh::container_t<hy::entity_t>& entities,
  const std::vector<thh::handle_t>& handles);

} // namespace hy_ig
