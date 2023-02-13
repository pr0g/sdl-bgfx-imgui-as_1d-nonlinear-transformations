#pragma once

#include "bgfx/bgfx.h"
#include <as/as-vec.hpp>

#include <optional>
#include <string>

struct PosColorVertex
{
  as::vec3 position_;
  uint32_t abgr_;
};

bgfx::ShaderHandle createShader(const std::string& shader, const char* name);

std::optional<bgfx::ProgramHandle> createShaderProgram(
  const char* vert_shader_path, const char* frag_shader_path);
