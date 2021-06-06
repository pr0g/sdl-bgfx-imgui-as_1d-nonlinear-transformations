#include "hierarchy-scene.h"

#include <SDL.h>

void imgui_hierarchy_scene_t::setup(
  const bgfx::ViewId main_view, const bgfx::ViewId ortho_view,
  const uint16_t width, const uint16_t height)
{
  main_view_ = main_view;
  ortho_view_ = ortho_view;

  root_handles = demo::create_sample_entities(entities);
  interaction.select(root_handles.front(), entities, root_handles);

  simple_program.init(dbg::SimpleEmbeddedShaderArgs);
  instance_program.init(dbg::InstanceEmbeddedShaderArgs);
}

void imgui_hierarchy_scene_t::input(const SDL_Event& current_event)
{
}

void imgui_hierarchy_scene_t::update(debug_draw_t& debug_draw)
{
  // imgui hierarchy experiments
  hy_ig::imguiInteractionDrawListHierarchy(
    entities, interaction, root_handles);
  hy_ig::imguiInteractionNormalHierarchy(
    entities, interaction, root_handles);
  hy_ig::imguiOnlyRecursiveHierarchy(entities, root_handles);

  bgfx::touch(main_view_);
}

void imgui_hierarchy_scene_t::teardown()
{
  simple_program.deinit();
  instance_program.deinit();
}
