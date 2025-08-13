// src/Engine/ShaderUtils.h
#pragma once
#include <bgfx/bgfx.h>

// 加载 .bin 着色器文件，rel 是相对路径（例如 "vs_simple.bin"）
bgfx::ShaderHandle loadShaderFile(const char* rel);

