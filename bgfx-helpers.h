#pragma once

#include "bgfx/bgfx.h"

#include <string>
#include <optional>

bgfx::ShaderHandle createShader(const std::string& shader, const char* name);

std::optional<bgfx::ProgramHandle> createShaderProgram(
  const char* vert_shader_path, const char* frag_shader_path);
