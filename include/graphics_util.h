#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace graphics_util {
	enum class TextureColorSpace {
		Linear,
		Srgb
	};

	struct TextureImage {
		int width = 0;
		int height = 0;
		int channels = 0;
		std::vector<unsigned char> pixels;
	};

	unsigned compile_shader(unsigned type, const char* source);
	unsigned compile_shader_from_file(unsigned type, const std::string& path);
	unsigned create_shader_program_from_files(
		const std::string& vertex_shader_path,
		const std::string& fragment_shader_path);
	uint32_t load_texture_2d(
		const std::string& filename,
		TextureColorSpace color_space = TextureColorSpace::Linear);
	TextureImage load_texture_image(const std::string& filename);
	uint32_t create_texture_2d(
		const TextureImage& image,
		TextureColorSpace color_space = TextureColorSpace::Linear);
}
