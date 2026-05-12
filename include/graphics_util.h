#pragma once

#include <cstdint>
#include <string>

namespace graphics_util {
	unsigned compile_shader(unsigned type, const char* source);
	unsigned compile_shader_from_file(unsigned type, const std::string& path);
	unsigned create_shader_program_from_files(
		const std::string& vertex_shader_path,
		const std::string& fragment_shader_path);
	uint32_t load_texture_2d(const std::string& filename);
}
