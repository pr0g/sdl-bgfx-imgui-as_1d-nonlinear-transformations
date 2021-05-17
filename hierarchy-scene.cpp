#include "hierarchy-scene.h"

#include <SDL.h>

void imgui_hierarchy_scene_t::setup(const uint16_t width, const uint16_t height)
{
  // move
  root_handles = demo::create_sample_entities(entities);
  interaction.select(root_handles.front(), entities, root_handles);

  // cornflower clear colour
  bgfx::setViewClear(
    main_view_, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
  bgfx::setViewRect(main_view_, 0, 0, width, height);

  simple_program.init(dbg::SimpleEmbeddedShaderArgs);
  instance_program.init(dbg::InstanceEmbeddedShaderArgs);
}

void imgui_hierarchy_scene_t::input(const SDL_Event& current_event)
{
  if (current_event.type == SDL_QUIT) {
    quit_ = true;
    return;
  }
}

void imgui_hierarchy_scene_t::update(debug_draw_t& debug_draw)
{
  // imgui hierarchy experiments
  hy_ig::imgui_interaction_draw_list_hierarchy(
    entities, interaction, root_handles);
  hy_ig::imgui_interaction_normal_hierarchy(
    entities, interaction, root_handles);
  hy_ig::imgui_only_recursive_hierarchy(entities, root_handles);

  bgfx::touch(main_view_);
}

void imgui_hierarchy_scene_t::teardown()
{
  simple_program.deinit();
  instance_program.deinit();
}
