#include "graphics_util.h"

#include <algorithm>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include <stb_image.h>


namespace graphics_util {

	unsigned compile_shader(unsigned type, const char* source) {
		unsigned shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		int succeed;
		char log_buf[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &succeed);
		if (!succeed) {
			glGetShaderInfoLog(shader, 512, nullptr, log_buf);
			std::cout << "Err: Shader compile failed: " << log_buf << std::endl;
			return 0;
		}

		return shader;
	}

	unsigned compile_shader_from_file(unsigned type, const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			std::cout << "Err: Failed to open shader file: " << path << std::endl;
			return 0;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		const std::string source = buffer.str();
		return compile_shader(type, source.c_str());
	}

	unsigned create_shader_program_from_files(
		const std::string& vertex_shader_path,
		const std::string& fragment_shader_path) {
		unsigned vertex_shader = compile_shader_from_file(GL_VERTEX_SHADER, vertex_shader_path);
		if (vertex_shader == 0) {
			return 0;
		}

		unsigned fragment_shader = compile_shader_from_file(GL_FRAGMENT_SHADER, fragment_shader_path);
		if (fragment_shader == 0) {
			glDeleteShader(vertex_shader);
			return 0;
		}

		unsigned shader_program = glCreateProgram();
		glAttachShader(shader_program, vertex_shader);
		glAttachShader(shader_program, fragment_shader);
		glLinkProgram(shader_program);

		int succeed = 0;
		glGetProgramiv(shader_program, GL_LINK_STATUS, &succeed);
		if (!succeed) {
			char log_buf[512];
			glGetProgramInfoLog(shader_program, 512, nullptr, log_buf);
			std::cout << "Err: Shader program link failed: " << log_buf << std::endl;
			glDeleteProgram(shader_program);
			shader_program = 0;
		}

		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
		return shader_program;
	}

	int get_uniform_location(const uint32_t shader_program, const char* name) {
		return glGetUniformLocation(shader_program, name);
	}

	void set_uniform_int(const int location, const int value) {
		glUniform1i(location, value);
	}

	void set_uniform_float(const int location, const float value) {
		glUniform1f(location, value);
	}

	void set_uniform_vec3(const int location, const glm::vec3& value) {
		glUniform3fv(location, 1, &value[0]);
	}

	void set_uniform_vec4(const int location, const glm::vec4& value) {
		glUniform4fv(location, 1, &value[0]);
	}

	void set_uniform_mat4(const int location, const glm::mat4& value) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
	}

	TextureImage load_texture_image(const std::string& filename) {
		TextureImage image{};
		if (filename.empty()) {
			return image;
		}

		int tex_width = 0;
		int tex_height = 0;
		int tex_channels = 0;

		stbi_set_flip_vertically_on_load(false);
		unsigned char* data = stbi_load(
			filename.c_str(),
			&tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
		if (data == nullptr) {
			std::cout << "Failed to load texture: " << filename << std::endl;
			return image;
		}

		image.width = tex_width;
		image.height = tex_height;
		image.channels = STBI_rgb_alpha;
		const std::size_t pixel_count = static_cast<std::size_t>(tex_width) *
			static_cast<std::size_t>(tex_height) * STBI_rgb_alpha;
		image.pixels.assign(data, data + pixel_count);
		stbi_image_free(data);
		return image;
	}

	uint32_t create_texture_2d(const TextureImage& image, const TextureColorSpace color_space) {
		if (image.width <= 0 || image.height <= 0 || image.pixels.empty()) {
			return 0;
		}

		uint32_t texture = 0;
		GLint prev_unpack_alignment = 4;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack_alignment);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		const GLint internal_format = color_space == TextureColorSpace::Srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		glTexImage2D(
			GL_TEXTURE_2D, 0, internal_format,
			image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());
		glGenerateMipmap(GL_TEXTURE_2D);
		glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack_alignment);

		return texture;
	}

	uint32_t create_cubemap_texture(const CubemapTextureDesc& desc) {
		if (desc.size <= 0 || desc.internal_format == 0 || desc.format == 0 || desc.type == 0) {
			std::cout << "Err: Invalid cubemap texture description." << std::endl;
			return 0;
		}

		uint32_t texture = 0;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
		for (int face = 0; face < 6; ++face) {
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0,
				static_cast<GLint>(desc.internal_format),
				desc.size, desc.size, 0,
				desc.format, desc.type, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, desc.generate_mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (desc.generate_mipmaps) {
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}

		return texture;
	}

	int create_cubemap_capture_target(const int size, CubemapCaptureTarget* out_target) {
		if (out_target == nullptr || size <= 0) {
			std::cout << "Err: Invalid cubemap capture target." << std::endl;
			return -1;
		}

		destroy_cubemap_capture_target(out_target);

		glGenFramebuffers(1, &out_target->framebuffer);
		glGenRenderbuffers(1, &out_target->depth_renderbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, out_target->framebuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, out_target->depth_renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
		glFramebufferRenderbuffer(
			GL_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT,
			GL_RENDERBUFFER,
			out_target->depth_renderbuffer);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "Err: Cubemap capture framebuffer is incomplete." << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			destroy_cubemap_capture_target(out_target);
			return -1;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		out_target->size = size;
		return 0;
	}

	void destroy_cubemap_capture_target(CubemapCaptureTarget* target) {
		if (target == nullptr) {
			return;
		}
		if (target->depth_renderbuffer != 0) {
			glDeleteRenderbuffers(1, &target->depth_renderbuffer);
			target->depth_renderbuffer = 0;
		}
		if (target->framebuffer != 0) {
			glDeleteFramebuffers(1, &target->framebuffer);
			target->framebuffer = 0;
		}
		target->size = 0;
	}

	void bind_cubemap_face(
		const CubemapCaptureTarget& target,
		const uint32_t cubemap_texture,
		const int face_index,
		const int mip_level) {
		if (target.framebuffer == 0 || target.size <= 0 || cubemap_texture == 0 ||
			face_index < 0 || face_index >= 6 || mip_level < 0) {
			std::cout << "Err: Invalid cubemap face bind request." << std::endl;
			return;
		}

		const int mip_size = std::max(1, target.size >> mip_level);
		glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
		glViewport(0, 0, mip_size, mip_size);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index,
			cubemap_texture,
			mip_level);
	}

	uint32_t load_texture_2d(const std::string& filename, const TextureColorSpace color_space) {
		return create_texture_2d(load_texture_image(filename), color_space);
	}

}
