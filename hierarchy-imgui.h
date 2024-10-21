#pragma once

#include <hierarchy/entity.hpp>

namespace hy_ig
{

void imguiInteractionDrawListHierarchy(
  const thh::handle_vector_t<hy::entity_t>& entities,
  hy::interaction_t& interaction,
  const std::vector<thh::handle_t>& root_handles);

void imguiInteractionNormalHierarchy(
  const thh::handle_vector_t<hy::entity_t>& entities,
  hy::interaction_t& interaction,
  const std::vector<thh::handle_t>& root_handles);

void imguiOnlyRecursiveHierarchy(
  const thh::handle_vector_t<hy::entity_t>& entities,
  const std::vector<thh::handle_t>& handles);

} // namespace hy_ig
