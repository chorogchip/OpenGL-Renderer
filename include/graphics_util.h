#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

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

	struct HdrTextureImage {
		int width = 0;
		int height = 0;
		int channels = 0;
		std::vector<float> pixels;
	};

	struct CubemapTextureDesc {
		int size = 0;
		uint32_t internal_format = 0;
		uint32_t format = 0;
		uint32_t type = 0;
		int mip_levels = 1;
		bool generate_mipmaps = false;
	};

	struct CubemapCaptureTarget {
		uint32_t framebuffer = 0;
		uint32_t depth_renderbuffer = 0;
		int size = 0;
	};

	struct CubemapFaceRenderInfo {
		int face_index = 0;
		glm::mat4 projection = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
	};

	unsigned compile_shader(unsigned type, const char* source);
	unsigned compile_shader_from_file(unsigned type, const std::string& path);
	unsigned create_shader_program_from_files(
		const std::string& vertex_shader_path,
		const std::string& fragment_shader_path);
	int get_uniform_location(uint32_t shader_program, const char* name);
	void set_uniform_int(int location, int value);
	void set_uniform_float(int location, float value);
	void set_uniform_vec3(int location, const glm::vec3& value);
	void set_uniform_vec4(int location, const glm::vec4& value);
	void set_uniform_mat4(int location, const glm::mat4& value);
	uint32_t load_texture_2d(
		const std::string& filename,
		TextureColorSpace color_space = TextureColorSpace::Linear);
	TextureImage load_texture_image(const std::string& filename);
	HdrTextureImage load_hdr_texture_image(const std::string& filename);
	uint32_t create_texture_2d(
		const TextureImage& image,
		TextureColorSpace color_space = TextureColorSpace::Linear);
	uint32_t create_hdr_texture_2d(const HdrTextureImage& image);
	uint32_t load_hdr_texture_2d(const std::string& filename);
	int cubemap_mip_count(int size);
	int cubemap_mip_size(int size, int mip_level);
	const glm::mat4& cubemap_capture_projection();
	const glm::mat4& cubemap_capture_view(int face_index);
	uint32_t create_cubemap_texture(const CubemapTextureDesc& desc);
	int create_cubemap_capture_target(int size, CubemapCaptureTarget* out_target);
	void destroy_cubemap_capture_target(CubemapCaptureTarget* target);
	void bind_cubemap_face(
		const CubemapCaptureTarget& target,
		uint32_t cubemap_texture,
		int face_index,
		int mip_level = 0);
	void render_cubemap_faces(
		const CubemapCaptureTarget& target,
		uint32_t cubemap_texture,
		const std::function<void(const CubemapFaceRenderInfo&)>& render_face,
		int mip_level = 0);
}
